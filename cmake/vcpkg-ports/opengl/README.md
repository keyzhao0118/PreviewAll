# Windows SDK OpenGL

The build files are copied unchanged from microsoft/vcpkg commit
`68112cf89d41fdb70c5910f7581707135be298f4`, `ports/opengl` (MIT; see LICENSE.txt).
Only the manifest differs: this overlay is limited to native Windows and omits
the `opengl-registry` dependency. The port still installs `gl.h`, `glu.h`,
`OpenGL32.Lib`, and `GlU32.Lib` from the installed Windows SDK, just as upstream.

PreviewAll targets Windows desktop OpenGL; Qt supplies its own extension headers.
The full Khronos API/specification repository is unnecessary for this build and
was a download bottleneck. If the application later needs Khronos registry XML,
GLES or other registry headers directly, remove this overlay and use upstream.
Review this overlay when updating the vcpkg baseline.
