#include <QApplication>
#include <QPlainTextEdit>
#include <QDebug>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/Repository>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QPlainTextEdit e;

	KSyntaxHighlighting::Repository repo;
	auto syntax = repo.definitionForName("C++");
	if (syntax.isValid())
	{
		auto theme = repo.defaultTheme();
		KSyntaxHighlighting::SyntaxHighlighter* highlighter =
			new KSyntaxHighlighting::SyntaxHighlighter(e.document());
		highlighter->setDefinition(syntax);
		highlighter->setTheme(theme);
	}

	e.setFont(QFont("Consolas", 11));
	e.setPlainText(
		"#include <iostream>\n"
		"int main() {\n"
		"   std::cout << \"hello\" << std::endl;\n"
		"   return 0;\n"
		"}"
	);

	for (const auto& def : repo.definitions()) {
		if (!def.isValid())
			continue;

		qDebug() << "Name:" << def.name();
		qDebug() << "Extensions:" << def.extensions();
		qDebug() << "Mime Types:" << def.mimeTypes();
		qDebug() << "--------------------------------";
	}



	e.show();
	app.exec();

	return 0;
}
