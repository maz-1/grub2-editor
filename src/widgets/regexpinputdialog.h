#ifndef REGEXPINPUTDIALOG_H
#define REGEXPINPUTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

class QLabel;
class QLineEdit;
class QDialogButtonBox;
class QRegExpValidator;

class RegExpInputDialog : public QDialog
{
Q_OBJECT
public:
explicit RegExpInputDialog(QWidget *parent = 0, Qt::WindowFlags=0);

void setTitle(const QString &title);
void setLabelText(const QString &label);
void setText(const QString &text);
void setRegExp(const QRegExp &regExp);

QString getLabelText();
QString getText();

static QString getText(QWidget *parent, const QString &title, const QString &label, const QString &text, const QRegExp &regExp, bool *ok, Qt::WindowFlags flags=0);

signals:

private slots:
void checkValid(const QString &text);

private:
QLabel *label;
QLineEdit *text;
QDialogButtonBox *buttonBox;
QRegExp regExp;
QRegExpValidator *validator;
};

#endif // REGEXPINPUTDIALOG_H