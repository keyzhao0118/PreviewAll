"""Analyze recorded Handler events without launching or driving a preview host."""
import collections
import csv
import json
from pathlib import Path, PureWindowsPath

root = Path(__file__).resolve().parent
frequency = json.loads((root / 'metadata.json').read_text(encoding='utf-8'))['qpc_frequency']
rows = []
with (root / 'events.tsv').open(encoding='utf-8') as stream:
    for line, fields in enumerate(csv.reader(stream, delimiter='\t'), 1):
        if len(fields) != 9:
            raise ValueError(f'Incomplete trace line {line}: {fields}')
        tick, pid, tid, instance, address, event, hwnd, result, detail = fields
        rows.append(dict(line=line, tick=int(tick), pid=int(pid), tid=int(tid),
                         instance=int(instance), address=address, event=event,
                         hwnd=int(hwnd), result=result, detail=detail))


def milliseconds(start, end):
    return round((end['tick'] - start['tick']) * 1000 / frequency, 4)


def category(path):
    suffix = PureWindowsPath(path).suffix.lower()
    if suffix in {'.zip', '.rar', '.7z'}:
        return 'archive'
    if suffix in {'.md', '.markdown'}:
        return 'markdown'
    if suffix in {'.png', '.jpg', '.jpeg', '.tif', '.tiff', '.bmp', '.webp', '.ico', '.svg', '.gif'}:
        return 'image'
    return 'unknown'


groups = collections.defaultdict(list)
for row in rows:
    groups[(row['pid'], row['instance'])].append(row)

instances, transitions, calls, scope_errors = [], [], [], []
for key, events in groups.items():
    stacks = collections.defaultdict(list)
    instance_calls = []
    current_parent = None
    for event in events:
        name = event['event']
        stack = stacks[event['tid']]
        if name.endswith('.begin'):
            stack.append(event)
        elif name.endswith('.end'):
            if not stack or stack[-1]['event'][:-6] != name[:-4]:
                scope_errors.append(event)
                continue
            begin = stack.pop()
            method = name[:-4]
            call = dict(method=method, begin=begin, end=event,
                        duration_ms=milliseconds(begin, event),
                        in_destructor=any(e['event'] == 'destruct.begin' for e in stack))
            if method == 'SetWindow' and event['result'] == '0':
                current_parent = begin['detail']
            call['parent'] = current_parent
            instance_calls.append(call)
    previews = [c for c in instance_calls if c['method'] == 'DoPreview']
    unloads = [c for c in instance_calls if c['method'] == 'Unload' and not c['in_destructor']]
    destructors = [c for c in instance_calls if c['method'] == 'destruct']
    item = dict(pid=key[0], instance=key[1], address=events[0]['address'],
                threads=sorted({e['tid'] for e in events}),
                constructors=sum(e['event'] == 'construct' for e in events),
                destructors=len(destructors), preview_count=len(previews), external_unloads=len(unloads),
                extensions=sorted({PureWindowsPath(c['begin']['detail']).suffix for c in previews}),
                parents=sorted({c['parent'] for c in previews if c['parent']}),
                first_line=events[0]['line'], last_line=events[-1]['line'],
                zero_releases=[e for e in events if e['event'] == 'Release' and e['result'] == '0'],
                unfinished_calls=[e for stack in stacks.values() for e in stack])
    if destructors:
        destructor = destructors[0]['begin']
        item['destructor_thread'] = destructor['tid']
        item['destructor_line'] = destructor['line']
        if unloads:
            item['last_external_unload_to_destructor_ms'] = milliseconds(unloads[-1]['end'], destructor)
    instances.append(item)
    for old, new in zip(previews, previews[1:]):
        between = [c for c in unloads if old['end']['tick'] <= c['begin']['tick'] < new['begin']['tick']]
        initializations = [c for c in instance_calls if c['method'] == 'Initialize'
                           and old['end']['tick'] <= c['begin']['tick'] < new['begin']['tick']]
        unload = between[-1] if between else None
        transitions.append(dict(pid=key[0], instance=key[1],
            previous=PureWindowsPath(old['begin']['detail']).name,
            current=PureWindowsPath(new['begin']['detail']).name,
            kind=category(old['begin']['detail'])+'->'+category(new['begin']['detail']),
            same_thread=bool(unload) and old['begin']['tid'] == unload['begin']['tid'] == new['begin']['tid'],
            same_parent=old['parent'] == new['parent'],
            unload_ended_before_initialize=bool(unload and initializations)
                and unload['end']['tick'] <= initializations[-1]['begin']['tick'],
            unload_ended_before_preview=bool(unload) and unload['end']['tick'] <= new['begin']['tick'],
            unload_duration_ms=unload['duration_ms'] if unload else None,
            gap_ms=milliseconds(unload['end'], new['begin']) if unload else None,
            from_line=old['begin']['line'], to_line=new['begin']['line']))
    calls.extend(instance_calls)

summary = dict(lines=len(rows), instances=instances,
    events=dict(collections.Counter(e['event'] for e in rows)),
    transitions=len(transitions),
    transitions_by_kind=dict(collections.Counter(t['kind'] for t in transitions)),
    nonserial_or_missing_unload=[t for t in transitions if not (t['same_thread']
        and t['unload_ended_before_initialize'] and t['unload_ended_before_preview'])],
    failed_preview=[c['end'] for c in calls if c['method']=='DoPreview' and c['end']['result']!='0'],
    failed_unload=[c['end'] for c in calls if c['method']=='Unload' and c['end']['result']!='0'],
    scope_errors=scope_errors)
(root / 'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))
