#include <QtGui>
#include "regexpinputdialog.h"

RegExpInputDialog::RegExpInputDialog(QWidget *parent, Qt::WindowFlags flags) :
QDialog(parent)
{
if(flags!=0) setWindowFlags(flags);
QVBoxLayout *l=new QVBoxLayout(this);

label=new QLabel(this);

regExp=QRegExp("*");
regExp.setPatternSyntax(QRegExp::Wildcard);
validator=new QRegExpValidator(regExp);

text=new QLineEdit(this);
text->setValidator(validator);
connect(text, SIGNAL(textChanged(QString)), this, SLOT(checkValid(QString)));

buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal, this);
connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

l->addWidget(label);
l->addWidget(text);
l->addWidget(buttonBox);
}

void RegExpInputDialog::setTitle(const QString &title){ setWindowTitle(title); }
void RegExpInputDialog::setLabelText(const QString &label){ this->label->setText(label); }
void RegExpInputDialog::setText(const QString &text){ this->text->setText(text); }

void RegExpInputDialog::setRegExp(const QRegExp &regExp){
validator->setRegExp(regExp);
checkValid(text->text());
}

QString RegExpInputDialog::getLabelText(){ return label->text(); }
QString RegExpInputDialog::getText(){ return text->text(); }

void RegExpInputDialog::checkValid(const QString &text){
QString _text=QString(text);
int pos=0;
bool valid=validator->validate(_text, pos)==QRegExpValidator::Acceptable;
buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

QString RegExpInputDialog::getText(QWidget *parent, const QString &title, const QString &label, const QString &text, const QRegExp &regExp, bool *ok, Qt::WindowFlags flags){
RegExpInputDialog *r=new RegExpInputDialog(parent, flags);
r->setTitle(title);
r->setLabelText(label);
r->setText(text);
r->setRegExp(regExp);
*ok=r->exec()==QDialog::Accepted;
if(*ok) return r->getText();
else return QString();
}