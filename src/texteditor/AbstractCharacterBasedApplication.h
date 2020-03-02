#ifndef ABSTRACTCHARACTERBASEDAPPLICATION_H
#define ABSTRACTCHARACTERBASEDAPPLICATION_H

#include <QList>
#include <QRect>
#include <QString>
#include <QByteArray>
#include <memory>
#include <vector>
#include <functional>
#include <QKeyEvent>
#include <QColor>

namespace EscapeCode {
enum EscapeCode {
	Up = 0x1b5b4100,
	Down = 0x1b5b4200,
	Right = 0x1b5b4300,
	Left = 0x1b5b4400,
	Home = 0x1b4f4800,
	End = 0x1b4f4600,
	Insert = 0x1b5b327e,
	Delete = 0x1b5b337e,
	PageUp = 0x1b5b357e,
	PageDown = 0x1b5b367e,
};
}

class Document {
public:
	struct CharAttr_ {
		size_t offset = 0;
		int color = -1;
	};
	struct Line {
		enum Type {
			Unknown,
			Normal,
			Add,
			Del,
		};
		int type = Unknown;
		int hunk_number = -1;
		int line_number = 0;
		size_t byte_offset = 0;
		QByteArray text;

		Line() = default;

		explicit Line(QByteArray const &ba)
			: type(Normal)
			, text(ba)
		{
		}

		explicit Line(std::string const &str, Type type = Normal)
			: type(type)
		{
			text.append(str.c_str(), str.size());
		}
	};

	QList<Line> lines;
	void retrieveLastText(std::vector<char> *out, int maxlen) const;
};

class TextEditorEngine {
public:
	Document document;
	TextEditorEngine()
	{
		document.lines.push_back(Document::Line(QByteArray()));
	}
};

struct SelectionAnchor {
	enum Enabled {
		No,
		EnabledHard,
		EnabledEasy,
	};

	Enabled enabled = No;
	int row = 0;
	int col = 0;
	int compare(SelectionAnchor const &a) const
	{
		if (enabled && a.enabled) {
			if (row < a.row) return -1;
			if (row > a.row) return 1;
			if (col < a.col) return -1;
			if (col > a.col) return 1;
		} else {
			if (a.enabled) return -1;
			if (enabled) return 1;
		}
		return 0;
	}
	bool operator == (SelectionAnchor const &a) const
	{
		return compare(a) == 0;
	}
	bool operator != (SelectionAnchor const &a) const
	{
		return compare(a) != 0;
	}
	bool operator < (SelectionAnchor const &a) const
	{
		return compare(a) < 0;
	}
	bool operator > (SelectionAnchor const &a) const
	{
		return compare(a) > 0;
	}
	bool operator <= (SelectionAnchor const &a) const
	{
		return compare(a) <= 0;
	}
	bool operator >= (SelectionAnchor const &a) const
	{
		return compare(a) >= 0;
	}
};

using TextEditorEnginePtr = std::shared_ptr<TextEditorEngine>;

struct TextEditorContext {
	QRect cursor_rect;
	bool single_line = false;
	int current_row = 0;
	int current_col = 0;
	int current_col_hint = 0;
	int saved_row = 0;
	int saved_col = 0;
	int saved_col_hint = 0;
	int current_char_span = 1;
	int scroll_row_pos = 0;
	int scroll_col_pos = 0;
	int viewport_org_x = 0;
	int viewport_org_y = 1;
	int viewport_width = 80;
	int viewport_height = 23;
	int tab_span = 4;
	int bottom_line_y = -1;
	TextEditorEnginePtr engine;
};

using DialogHandler = std::function<void (bool, QString const &)>;

class AbstractCharacterBasedApplication {
public:
	static const int LEFT_MARGIN = 8;
	static const int RIGHT_MARGIN = 10;

	enum class WriteMode {
		Insert,
		Overwrite,
	};

	enum class State {
		Normal,
		Exit,
	};

	struct CharAttr {
		uint16_t index;
		uint16_t flags = 0;
		QColor color;
		CharAttr(int index = Normal)
			: index(index)
		{
		}
		bool operator == (CharAttr const &r) const
		{
			return index == r.index && color == r.color;
		}
		bool operator != (CharAttr const &r) const
		{
			return !operator == (r);
		}
		enum Index {
			Normal,
			Invert,
			Hilite,
		};
		enum Flag {
			Selected = 0x0001,
			CurrentLine = 0x0002,
		};
	};

	struct Option {
		CharAttr char_attr;
		QRect clip;
	};

	struct Character {
		uint16_t c = 0;
		CharAttr a;
	};

	struct Char {
		unsigned int pos = 0;
		uint32_t unicode = 0;
		Char() = default;
		Char(uint32_t unicode, unsigned int pos)
			: unicode(unicode)
			, pos(pos)
		{

		}
//		operator unsigned int () const = delete;
	};

	enum LineFlag {
		LineChanged = 1,
	};

	static int charWidth(uint32_t c);

	class FormattedLine {
	public:
		QString text;
		enum Attr {
			StyleID = 0x00ffffff,
			Selected = 0x01000000,
		};
		uint32_t atts;
		FormattedLine(QString const &text, int atts)
			: text(text)
			, atts(atts)
		{
		}
		bool isSelected() const
		{
			return atts & Selected;
		}
	};

	QList<FormattedLine> formatLine(const Document::Line &line, int tab_span, int anchor_a = -1, int anchor_b = -1) const;
private:
	struct Private;
	Private *m;
protected:
	SelectionAnchor selection_anchor_0;
	SelectionAnchor selection_anchor_1;
protected:

	std::vector<Character> *char_screen();
	std::vector<Character> const *char_screen() const;
	std::vector<uint8_t> *line_flags();

	void initEditor();

	void fetchCurrentLine() const;
	QByteArray fetchLine(int row) const;
	void clearParsedLine();

	int cursorX() const;
	int cursorY() const;

	int editorViewportWidth() const;
	int editorViewportHeight() const;

	virtual int print(int x, int y, QString const &text, Option const &opt);

	std::shared_ptr<TextEditorContext> editor_cx;
	std::shared_ptr<TextEditorContext> dialog_cx;

	TextEditorContext *cx();
	TextEditorContext const *cx() const;

	Document *document();
	int documentLines() const;

	bool isSingleLineMode() const;

	void ensureCurrentLineVisible();
	int decideColumnScrollPos() const;

	int calcVisualWidth(Document::Line const &line) const;

	int leftMargin_() const;

	void makeBuffer();
	int printArea(const TextEditorContext *cx, SelectionAnchor const *sel_a = nullptr, SelectionAnchor const *sel_b = nullptr);

	int calcIndexToColumn(const std::vector<Char> &vec, int index) const;

	virtual void updateVisibility(bool ensure_current_line_visible, bool change_col, bool auto_scroll) = 0;
	void commitLine(const std::vector<Char> &vec);

	void doDelete();
	void doBackspace();

	bool isDialogMode();
	void setDialogMode(bool f);
	void closeDialog(bool result);
	void setDialogOption(QString const &title, QString const &value, const DialogHandler &handler);
	void execDialog(QString const &dialog_title, const QString &dialog_value, const DialogHandler &handler);
	void toggleSelectionAnchor();
private:
	int internalParseLine(const QByteArray &parsed_line, std::vector<Char> *vec, int increase_hint) const;
	void internalWrite(const ushort *begin, const ushort *end);
	void pressLetterWithControl(int c);
	void invalidateAreaBelowTheCurrentLine();
	void onQuit();
	void onOpenFile();
	void onSaveFile();
	void printInvertedBar(int x, int y, char const *text, int padchar);
	SelectionAnchor currentAnchor(SelectionAnchor::Enabled enabled);
	enum class EditOperation {
		Cut,
		Copy,
	};
	void editSelected(EditOperation op, std::vector<Char> *cutbuffer);
	void deselect();
	int calcColumnToIndex(int column);
	void edit_(EditOperation op);
	bool isCurrentLineWritable() const;
	void initEngine(const std::shared_ptr<TextEditorContext>& cx);
	void writeCR();
	bool deleteIfSelected();
	static int findSyntax(const QList<Document::CharAttr_> *list, size_t offset);
	static void insertSyntax(QList<Document::CharAttr_> *list, size_t offset, const Document::CharAttr_ &a);
protected:
	void parseLine(std::vector<Char> *vec, int increase_hint, bool force);
	int parseLine2(int row, std::vector<Char> *vec) const;
	QByteArray parsedLine() const;
	void setCursorRow(int row, bool auto_scroll = true, bool by_mouse = false);
	void setCursorCol(int col, bool auto_scroll = true, bool by_mouse = false);
	void setCursorPos(int row, int col);
	void setCursorColByIndex(const std::vector<Char> &vec, int col_index);
	int nextTabStop(int x) const;
	int scrollBottomLimit() const;
	bool isPaintingSuppressed() const;
	void setPaintingSuppressed(bool f);
	void scrollRight();
	void scrollLeft();

	void addNewLineToBottom();
	void appendNewLine(std::vector<Char> *vec);
	void writeNewLine();
	void updateCursorPos(bool auto_scroll);

	QString statusLine() const;

	void preparePaintScreen();
	void setRecentlyUsedPath(QString const &path);
	QString recentlyUsedPath();
	void clearRect(int x, int y, int w, int h);
	void paintLineNumbers(std::function<void(int, QString, Document::Line const *line)> const &draw);
	bool isAutoLayout() const;
	void invalidateArea(int top_y = 0);
	void savePos();
	void restorePos();
public:
	virtual void layoutEditor();
	void scrollUp();
	void scrollDown();
	void moveCursorOut();
	void moveCursorHome();
	void moveCursorEnd();
	void moveCursorUp();
	void moveCursorDown();
	void moveCursorLeft();
	void moveCursorRight();
	void movePageUp();
	void movePageDown();
	void scrollToTop();

	AbstractCharacterBasedApplication();
	virtual ~AbstractCharacterBasedApplication();
	TextEditorEnginePtr engine();
	int screenWidth() const;
	int screenHeight() const;
	void setScreenSize(int w, int h, bool update_layout);
	void setTextEditorEngine(const TextEditorEnginePtr &e);
	void openFile(QString const &path);
	void saveFile(QString const &path);
	void loadExampleFile();
	void pressEnter();
	void pressEscape();
	State state() const;
	bool isLineNumberVisible() const;
	void showLineNumber(bool show, int left_margin = LEFT_MARGIN);
	void showHeader(bool f);
	void showFooter(bool f);
	void setAutoLayout(bool f);
	void setDocument(const QList<Document::Line> *source);
	void setSelectionAnchor(SelectionAnchor::Enabled enabled, bool update_anchor, bool auto_scroll);
	bool isSelectionAnchorEnabled() const;
	void setNormalTextEditorMode(bool f);
	void setToggleSelectionAnchorEnabled(bool f);
	void setReadOnly(bool f);
	bool isReadOnly() const;
	void editPaste();
	void editCopy();
	void editCut();
	void setWriteMode(WriteMode wm);
	bool isInsertMode() const;
	bool isOverwriteMode() const;
	void setTerminalMode(bool f);
	bool isTerminalMode() const;
	void moveToTop();
	void moveToBottom();
	bool isBottom() const;
	void setLineMargin(int n);
	void write(uint32_t c, bool by_keyboard);
	void write(char const *ptr, int len, bool by_keyboard);
	void write(std::string const &text);
	void write(QKeyEvent *e);
	void setTextCodec(QTextCodec *codec);
	void setCursorVisible(bool show);
	bool isCursorVisible();
	void setModifierKeys(Qt::KeyboardModifiers keymod);
	bool isControlModifierPressed() const;
	bool isShiftModifierPressed() const;
	void clearShiftModifier();
	void retrieveLastText(std::vector<char> *out, int maxlen) const;
	bool isChanged() const;
	void setChanged(bool f);
	void logicalMoveToBottom();
protected:
	void write_(char const *ptr, bool by_keyboard);
	void write_(QString const &text, bool by_keyboard);
	void makeColumnPosList(std::vector<int> *out);
};

class AbstractTextEditorApplication : public AbstractCharacterBasedApplication {

};


#endif // ABSTRACTCHARACTERBASEDAPPLICATION_H
