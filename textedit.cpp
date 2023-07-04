/*
 * This file is part of PlantUML QEditor.
 *
 * PlantUML QEditor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlantUML QEditor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PlantUML QEditor. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QDebug>
#include <QPainter>

#include "textedit.h"

// See to:

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(TextEdit *editor) : QWidget(editor)
    {
        this->editor = editor;
    }

    QSize sizeHint() const
    {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        editor->lineNumberAreaPaintEvent(event);
    }

private:
    TextEdit *editor;
};


TextEdit::TextEdit(QWidget *parent) :
    QPlainTextEdit(parent),
    _indentSize(4),
    _indentWithSpace(false),
    _autoIndent(true)
{
    setIndentSize(4);

    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));

    updateLineNumberAreaWidth(0);
}

void TextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);


    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

int TextEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}


void TextEdit::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            QPlainTextEdit::keyPressEvent(e);
            QTextCursor updateCursor = textCursor();

            // Auto-indent
            if (_autoIndent)
            {
                QTextBlock block          = updateCursor.block().previous();
                QString    data           = block.text();
                int        pos            = block.length();

                int i;
                if (pos >= data.length())
                    pos = data.length() - 1;

                for (i = pos; i >= 0; --i)
                {
                    if (data.at(i) == '\n')
                    {
                        break;
                    }
                }

                while ((i+1) < data.length() &&
                       data.at(i + 1).isSpace() &&
                       data.at(i + 1) != '\n' &&
                       data.at(i + 1) != '\r')
                {
                    updateCursor.insertText(data.at(i + 1));
                    i++;
                }
            }

            break;
        }

        case Qt::Key_Tab:
        case Qt::Key_Backtab:
        {
            QString indentLine;
            if (_indentWithSpace)
            {
                indentLine = indentLine.leftJustified(_indentSize, QChar(' '));
            }
            else
            {
                indentLine = QChar('\t');
            }

            QTextCursor currentTextCursor = textCursor();
            int selectionStart = currentTextCursor.selectionStart();
            int selectionEnd = currentTextCursor.selectionEnd();

            if (selectionStart == selectionEnd)
            {
                currentTextCursor.insertText(indentLine);
            }
            else
            {
                QTextBlock textBlock = document()->findBlock(selectionStart);

                while (textBlock.isValid() && textBlock.position() <= selectionEnd)
                {
                    currentTextCursor.setPosition(textBlock.position());

                    if (!e->modifiers().testFlag(Qt::ShiftModifier))
                    {
                        currentTextCursor.insertText(indentLine);
                        selectionEnd += indentLine.count();
                    }
                    else
                    {
                        QString text = currentTextCursor.block().text();
                        for (int i = 0; i < qMin(indentLine.count(), text.length()); ++i)
                        {
                            if (!text.at(i).isSpace())
                                break;

                            currentTextCursor.deleteChar();
                            selectionEnd--;
                        }
                    }

                    textBlock = textBlock.next();
                }
            }

            break;
        }

        default:
        {
            QPlainTextEdit::keyPressEvent(e);
        }
    }
}

void TextEdit::paintEvent(QPaintEvent *e)
{
    // Update tab stops
    QString indentLine = QString().leftJustified(_indentSize, QChar(' '));
    setTabStopDistance(fontMetrics().horizontalAdvance(indentLine));

    QPlainTextEdit::paintEvent(e);
}

void TextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void TextEdit::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void TextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
    {
        lineNumberArea->scroll(0, dy);
    }
    else
    {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect()))
    {
        updateLineNumberAreaWidth(0);
    }
}

