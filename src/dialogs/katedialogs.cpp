/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>

   Based on work of:
     Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katedialogs.h"

#include <ktexteditor_version.h>

#include "kateautoindent.h"
#include "katebuffer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "katemodeconfigpage.h"
#include "kateview.h"
#include "spellcheck/spellcheck.h"
#include "kateglobal.h"

// auto generated ui files
#include "ui_modonhdwidget.h"
#include "ui_textareaappearanceconfigwidget.h"
#include "ui_bordersappearanceconfigwidget.h"
#include "ui_navigationconfigwidget.h"
#include "ui_editconfigwidget.h"
#include "ui_indentationconfigwidget.h"
#include "ui_completionconfigtab.h"
#include "ui_opensaveconfigwidget.h"
#include "ui_opensaveconfigadvwidget.h"
#include "ui_spellcheckconfigwidget.h"

#include <KIO/Job>
#include <kjobwidgets.h>

#include <KCharsets>
#include <KColorCombo>
#include <KComboBox>
#include <KIconLoader>
#include "katepartdebug.h"
#include "kateabstractinputmodefactory.h"
#include <KShortcutsDialog>
#include <KLineEdit>
#include <KMessageBox>
#include <KProcess>
#include <KRun>
#include <KSeparator>
#include <KActionCollection>

#include <QFile>
#include <QMap>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>
#include <QTemporaryFile>
#include <QKeyEvent>
#include <QPainter>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QWhatsThis>
#include <QDomDocument>

// trailing slash is important
#define HLDOWNLOADPATH QStringLiteral("http://kate.kde.org/syntax/")

//END

//BEGIN KateIndentConfigTab
KateIndentConfigTab::KateIndentConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::IndentationConfigWidget();
    ui->setupUi(newWidget);

    ui->cmbMode->addItems(KateAutoIndent::listModes());

    ui->label->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    connect(ui->label, SIGNAL(linkActivated(QString)), this, SLOT(showWhatsThis(QString)));

    // What's This? help can be found in the ui file

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->cmbMode, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->rbIndentWithTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbIndentWithSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbIndentMixed, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbIndentWithTabs, SIGNAL(toggled(bool)), ui->sbIndentWidth, SLOT(setDisabled(bool)));

    connect(ui->chkKeepExtraSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkIndentPaste, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkBackspaceUnindents, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    connect(ui->sbTabWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->sbIndentWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

    connect(ui->rbTabAdvances, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbTabIndents, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbTabSmart, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateIndentConfigTab::~KateIndentConfigTab()
{
    delete ui;
}

void KateIndentConfigTab::slotChanged()
{
    if (ui->rbIndentWithTabs->isChecked()) {
        ui->sbIndentWidth->setValue(ui->sbTabWidth->value());
    }

    KateConfigPage::slotChanged();
}

void KateIndentConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateIndentConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateDocumentConfig::global()->configStart();

    KateDocumentConfig::global()->setKeepExtraSpaces(ui->chkKeepExtraSpaces->isChecked());
    KateDocumentConfig::global()->setBackspaceIndents(ui->chkBackspaceUnindents->isChecked());
    KateDocumentConfig::global()->setIndentPastedText(ui->chkIndentPaste->isChecked());
    KateDocumentConfig::global()->setIndentationWidth(ui->sbIndentWidth->value());
    KateDocumentConfig::global()->setIndentationMode(KateAutoIndent::modeName(ui->cmbMode->currentIndex()));
    KateDocumentConfig::global()->setTabWidth(ui->sbTabWidth->value());
    KateDocumentConfig::global()->setReplaceTabsDyn(ui->rbIndentWithSpaces->isChecked());

    if (ui->rbTabAdvances->isChecked()) {
        KateDocumentConfig::global()->setTabHandling(KateDocumentConfig::tabInsertsTab);
    } else if (ui->rbTabIndents->isChecked()) {
        KateDocumentConfig::global()->setTabHandling(KateDocumentConfig::tabIndents);
    } else {
        KateDocumentConfig::global()->setTabHandling(KateDocumentConfig::tabSmart);
    }

    KateDocumentConfig::global()->configEnd();
}

void KateIndentConfigTab::reload()
{
    ui->sbTabWidth->setSuffix(ki18np(" character", " characters"));
    ui->sbTabWidth->setValue(KateDocumentConfig::global()->tabWidth());
    ui->sbIndentWidth->setSuffix(ki18np(" character", " characters"));
    ui->sbIndentWidth->setValue(KateDocumentConfig::global()->indentationWidth());
    ui->chkKeepExtraSpaces->setChecked(KateDocumentConfig::global()->keepExtraSpaces());
    ui->chkIndentPaste->setChecked(KateDocumentConfig::global()->indentPastedText());
    ui->chkBackspaceUnindents->setChecked(KateDocumentConfig::global()->backspaceIndents());

    ui->rbTabAdvances->setChecked(KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabInsertsTab);
    ui->rbTabIndents->setChecked(KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabIndents);
    ui->rbTabSmart->setChecked(KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabSmart);

    ui->cmbMode->setCurrentIndex(KateAutoIndent::modeNumber(KateDocumentConfig::global()->indentationMode()));

    if (KateDocumentConfig::global()->replaceTabsDyn()) {
        ui->rbIndentWithSpaces->setChecked(true);
    } else {
        if (KateDocumentConfig::global()->indentationWidth() == KateDocumentConfig::global()->tabWidth()) {
            ui->rbIndentWithTabs->setChecked(true);
        } else {
            ui->rbIndentMixed->setChecked(true);
        }
    }

    ui->sbIndentWidth->setEnabled(!ui->rbIndentWithTabs->isChecked());
}

QString KateIndentConfigTab::name() const
{
    return i18n("Indentation");
}

//END KateIndentConfigTab

//BEGIN KateCompletionConfigTab
KateCompletionConfigTab::KateCompletionConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::CompletionConfigTab();
    ui->setupUi(newWidget);

    // What's This? help can be found in the ui file

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->chkAutoCompletionEnabled, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->gbWordCompletion, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->gbKeywordCompletion, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->minimalWordLength, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->removeTail, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateCompletionConfigTab::~KateCompletionConfigTab()
{
    delete ui;
}

void KateCompletionConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateCompletionConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateViewConfig::global()->setAutomaticCompletionInvocation(ui->chkAutoCompletionEnabled->isChecked());
    KateViewConfig::global()->setWordCompletion(ui->gbWordCompletion->isChecked());
    KateViewConfig::global()->setWordCompletionMinimalWordLength(ui->minimalWordLength->value());
    KateViewConfig::global()->setWordCompletionRemoveTail(ui->removeTail->isChecked());
    KateViewConfig::global()->setKeywordCompletion(ui->gbKeywordCompletion->isChecked());
    KateViewConfig::global()->configEnd();
}

void KateCompletionConfigTab::reload()
{
    ui->chkAutoCompletionEnabled->setChecked(KateViewConfig::global()->automaticCompletionInvocation());
    ui->gbWordCompletion->setChecked(KateViewConfig::global()->wordCompletion());
    ui->minimalWordLength->setValue(KateViewConfig::global()->wordCompletionMinimalWordLength());
    ui->gbKeywordCompletion->setChecked(KateViewConfig::global()->keywordCompletion());
    ui->removeTail->setChecked(KateViewConfig::global()->wordCompletionRemoveTail());
}

QString KateCompletionConfigTab::name() const
{
    return i18n("Auto Completion");
}

//END KateCompletionConfigTab

//BEGIN KateSpellCheckConfigTab
KateSpellCheckConfigTab::KateSpellCheckConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::SpellCheckConfigWidget();
    ui->setupUi(newWidget);

    // What's This? help can be found in the ui file
    reload();

    //
    // after initial reload, connect the stuff for the changed () signal

    m_sonnetConfigWidget = new Sonnet::ConfigWidget(this);
    connect(m_sonnetConfigWidget, SIGNAL(configChanged()), this, SLOT(slotChanged()));
    layout->addWidget(m_sonnetConfigWidget);

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateSpellCheckConfigTab::~KateSpellCheckConfigTab()
{
    delete ui;
}

void KateSpellCheckConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateSpellCheckConfigTab::apply()
{
    if (!hasChanged()) {
        // nothing changed, no need to apply stuff
        return;
    }
    m_changed = false;

    KateDocumentConfig::global()->configStart();
    m_sonnetConfigWidget->save();
    KateDocumentConfig::global()->configEnd();
    foreach (KTextEditor::DocumentPrivate *doc, KTextEditor::EditorPrivate::self()->kateDocuments()) {
        doc->refreshOnTheFlyCheck();
    }
}

void KateSpellCheckConfigTab::reload()
{
    // does nothing
}

QString KateSpellCheckConfigTab::name() const
{
    return i18n("Spellcheck");
}

//END KateSpellCheckConfigTab

//BEGIN KateNavigationConfigTab
KateNavigationConfigTab::KateNavigationConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us having more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::NavigationConfigWidget();
    ui->setupUi(newWidget);

    // What's This? Help is in the ui-files

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->cbTextSelectionMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkSmartHome, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkPagingMovesCursor, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->sbAutoCenterCursor, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkScrollPastEnd, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateNavigationConfigTab::~KateNavigationConfigTab()
{
    delete ui;
}

void KateNavigationConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateDocumentConfig::global()->configStart();

    KateDocumentConfig::global()->setSmartHome(ui->chkSmartHome->isChecked());

    KateViewConfig::global()->setAutoCenterLines(qMax(0, ui->sbAutoCenterCursor->value()));
    KateDocumentConfig::global()->setPageUpDownMovesCursor(ui->chkPagingMovesCursor->isChecked());

    KateViewConfig::global()->setPersistentSelection(ui->cbTextSelectionMode->currentIndex() == 1);

    KateViewConfig::global()->setScrollPastEnd(ui->chkScrollPastEnd->isChecked());

    KateDocumentConfig::global()->configEnd();
    KateViewConfig::global()->configEnd();
}

void KateNavigationConfigTab::reload()
{
    ui->cbTextSelectionMode->setCurrentIndex(KateViewConfig::global()->persistentSelection() ? 1 : 0);

    ui->chkSmartHome->setChecked(KateDocumentConfig::global()->smartHome());
    ui->chkPagingMovesCursor->setChecked(KateDocumentConfig::global()->pageUpDownMovesCursor());
    ui->sbAutoCenterCursor->setValue(KateViewConfig::global()->autoCenterLines());
    ui->chkScrollPastEnd->setChecked(KateViewConfig::global()->scrollPastEnd());
}

QString KateNavigationConfigTab::name() const
{
    return i18n("Text Navigation");
}

//END KateNavigationConfigTab

//BEGIN KateEditGeneralConfigTab
KateEditGeneralConfigTab::KateEditGeneralConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);
    ui = new Ui::EditConfigWidget();
    ui->setupUi(newWidget);

    QList<KateAbstractInputModeFactory *> inputModes = KTextEditor::EditorPrivate::self()->inputModeFactories();
    Q_FOREACH(KateAbstractInputModeFactory *fact, inputModes) {
        ui->cmbInputMode->addItem(fact->name(), static_cast<int>(fact->inputMode()));
    }

    reload();

    connect(ui->chkStaticWordWrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkShowStaticWordWrapMarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->sbWordWrap, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkAutoBrackets, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkSmartCopyCut, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->cmbInputMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));

    // "What's this?" help is in the ui-file

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateEditGeneralConfigTab::~KateEditGeneralConfigTab()
{
    delete ui;
}

void KateEditGeneralConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateDocumentConfig::global()->configStart();

    KateDocumentConfig::global()->setWordWrapAt(ui->sbWordWrap->value());
    KateDocumentConfig::global()->setWordWrap(ui->chkStaticWordWrap->isChecked());

    KateRendererConfig::global()->setWordWrapMarker(ui->chkShowStaticWordWrapMarker->isChecked());

    KateViewConfig::global()->setAutoBrackets(ui->chkAutoBrackets->isChecked());
    KateViewConfig::global()->setSmartCopyCut(ui->chkSmartCopyCut->isChecked());

    KateViewConfig::global()->setInputModeRaw(ui->cmbInputMode->currentData().toInt());

    KateDocumentConfig::global()->configEnd();
    KateViewConfig::global()->configEnd();
}

void KateEditGeneralConfigTab::reload()
{
    ui->chkStaticWordWrap->setChecked(KateDocumentConfig::global()->wordWrap());
    ui->chkShowStaticWordWrapMarker->setChecked(KateRendererConfig::global()->wordWrapMarker());
    ui->sbWordWrap->setSuffix(ki18ncp("Wrap words at (value is at 20 or larger)", " character", " characters"));
    ui->sbWordWrap->setValue(KateDocumentConfig::global()->wordWrapAt());
    ui->chkAutoBrackets->setChecked(KateViewConfig::global()->autoBrackets());
    ui->chkSmartCopyCut->setChecked(KateViewConfig::global()->smartCopyCut());

    const int id = static_cast<int>(KateViewConfig::global()->inputMode());
    ui->cmbInputMode->setCurrentIndex(ui->cmbInputMode->findData(id));
}

QString KateEditGeneralConfigTab::name() const
{
    return i18n("General");
}

//END KateEditGeneralConfigTab

//BEGIN KateEditConfigTab
KateEditConfigTab::KateEditConfigTab(QWidget *parent)
    : KateConfigPage(parent)
    , editConfigTab(new KateEditGeneralConfigTab(this))
    , navigationConfigTab(new KateNavigationConfigTab(this))
    , indentConfigTab(new KateIndentConfigTab(this))
    , completionConfigTab(new KateCompletionConfigTab(this))
    , spellCheckConfigTab(new KateSpellCheckConfigTab(this))
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    QTabWidget *tabWidget = new QTabWidget(this);

    // add all tabs
    tabWidget->insertTab(0, editConfigTab, editConfigTab->name());
    tabWidget->insertTab(1, navigationConfigTab, navigationConfigTab->name());
    tabWidget->insertTab(2, indentConfigTab, indentConfigTab->name());
    tabWidget->insertTab(3, completionConfigTab, completionConfigTab->name());
    tabWidget->insertTab(4, spellCheckConfigTab, spellCheckConfigTab->name());

    connect(editConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(navigationConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(indentConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(completionConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(spellCheckConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));

    int i = tabWidget->count();
    Q_FOREACH(KateAbstractInputModeFactory *factory, KTextEditor::EditorPrivate::self()->inputModeFactories()) {
        KateConfigPage *tab = factory->createConfigPage(this);
        if (tab) {
            m_inputModeConfigTabs << tab;
            tabWidget->insertTab(i, tab, tab->name());
            connect(tab, SIGNAL(changed()), this, SLOT(slotChanged()));
            i++;
        }
    }

    layout->addWidget(tabWidget);
    setLayout(layout);
}

KateEditConfigTab::~KateEditConfigTab()
{
    qDeleteAll(m_inputModeConfigTabs);
}

void KateEditConfigTab::apply()
{
    // try to update the rest of tabs
    editConfigTab->apply();
    navigationConfigTab->apply();
    indentConfigTab->apply();
    completionConfigTab->apply();
    spellCheckConfigTab->apply();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->apply();
    }
}

void KateEditConfigTab::reload()
{
    editConfigTab->reload();
    navigationConfigTab->reload();
    indentConfigTab->reload();
    completionConfigTab->reload();
    spellCheckConfigTab->reload();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->reload();
    }
}

void KateEditConfigTab::reset()
{
    editConfigTab->reset();
    navigationConfigTab->reset();
    indentConfigTab->reset();
    completionConfigTab->reset();
    spellCheckConfigTab->reset();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->reset();
    }
}

void KateEditConfigTab::defaults()
{
    editConfigTab->defaults();
    navigationConfigTab->defaults();
    indentConfigTab->defaults();
    completionConfigTab->defaults();
    spellCheckConfigTab->defaults();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->defaults();
    }
}

QString KateEditConfigTab::name() const
{
    return i18n("Editing");
}

QString KateEditConfigTab::fullName() const
{
    return i18n("Editing Options");
}

QIcon KateEditConfigTab::icon() const
{
    return QIcon::fromTheme(QStringLiteral("accessories-text-editor"));
}

//END KateEditConfigTab

//BEGIN KateViewDefaultsConfig
KateViewDefaultsConfig::KateViewDefaultsConfig(QWidget *parent)
    : KateConfigPage(parent)
    , textareaUi(new Ui::TextareaAppearanceConfigWidget())
    , bordersUi(new Ui::BordersAppearanceConfigWidget())
{
    QLayout *layout = new QVBoxLayout(this);
    QTabWidget *tabWidget = new QTabWidget(this);
    layout->addWidget(tabWidget);
    layout->setMargin(0);

    QWidget *textareaTab = new QWidget(tabWidget);
    textareaUi->setupUi(textareaTab);
    tabWidget->addTab(textareaTab, i18n("General"));

    QWidget *bordersTab = new QWidget(tabWidget);
    bordersUi->setupUi(bordersTab);
    tabWidget->addTab(bordersTab, i18n("Borders"));

    textareaUi->cmbDynamicWordWrapIndicator->addItem(i18n("Off"));
    textareaUi->cmbDynamicWordWrapIndicator->addItem(i18n("Follow Line Numbers"));
    textareaUi->cmbDynamicWordWrapIndicator->addItem(i18n("Always On"));

    // What's This? help is in the ui-file

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(textareaUi->gbWordWrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->cmbDynamicWordWrapIndicator, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(textareaUi->sbDynamicWordWrapDepth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowIndentationLines, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowWholeBracketExpression, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkAnimateBracketMatching, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkFoldFirstLine,  SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    connect(bordersUi->chkIconBorder, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarMarks, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarMiniMap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarMiniMapAll, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    bordersUi->chkScrollbarMiniMapAll->hide(); // this is temporary until the feature is done
    connect(bordersUi->spBoxMiniMapWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(bordersUi->chkLineNumbers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkShowLineModification, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkShowFoldingMarkers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->rbSortBookmarksByPosition, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->rbSortBookmarksByCreation, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->cmbShowScrollbars, SIGNAL(activated(int)), this, SLOT(slotChanged()));
}

KateViewDefaultsConfig::~KateViewDefaultsConfig()
{
    delete bordersUi;
    delete textareaUi;
}

void KateViewDefaultsConfig::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateRendererConfig::global()->configStart();

    KateViewConfig::global()->setDynWordWrap(textareaUi->gbWordWrap->isChecked());
    KateViewConfig::global()->setDynWordWrapIndicators(textareaUi->cmbDynamicWordWrapIndicator->currentIndex());
    KateViewConfig::global()->setDynWordWrapAlignIndent(textareaUi->sbDynamicWordWrapDepth->value());
    KateDocumentConfig::global()->setShowTabs(textareaUi->chkShowTabs->isChecked());
    KateDocumentConfig::global()->setShowSpaces(textareaUi->chkShowSpaces->isChecked());
    KateViewConfig::global()->setLineNumbers(bordersUi->chkLineNumbers->isChecked());
    KateViewConfig::global()->setIconBar(bordersUi->chkIconBorder->isChecked());
    KateViewConfig::global()->setScrollBarMarks(bordersUi->chkScrollbarMarks->isChecked());
    KateViewConfig::global()->setScrollBarMiniMap(bordersUi->chkScrollbarMiniMap->isChecked());
    KateViewConfig::global()->setScrollBarMiniMapAll(bordersUi->chkScrollbarMiniMapAll->isChecked());
    KateViewConfig::global()->setScrollBarMiniMapWidth(bordersUi->spBoxMiniMapWidth->value());
    KateViewConfig::global()->setFoldingBar(bordersUi->chkShowFoldingMarkers->isChecked());
    KateViewConfig::global()->setLineModification(bordersUi->chkShowLineModification->isChecked());
    KateViewConfig::global()->setShowScrollbars(bordersUi->cmbShowScrollbars->currentIndex());

    KateViewConfig::global()->setBookmarkSort(bordersUi->rbSortBookmarksByPosition->isChecked() ? 0 : 1);
    KateRendererConfig::global()->setShowIndentationLines(textareaUi->chkShowIndentationLines->isChecked());
    KateRendererConfig::global()->setShowWholeBracketExpression(textareaUi->chkShowWholeBracketExpression->isChecked());
    KateRendererConfig::global()->setAnimateBracketMatching(textareaUi->chkAnimateBracketMatching->isChecked());
    KateViewConfig::global()->setFoldFirstLine(textareaUi->chkFoldFirstLine->isChecked());

    KateRendererConfig::global()->configEnd();
    KateViewConfig::global()->configEnd();
}

void KateViewDefaultsConfig::reload()
{
    textareaUi->gbWordWrap->setChecked(KateViewConfig::global()->dynWordWrap());
    textareaUi->cmbDynamicWordWrapIndicator->setCurrentIndex(KateViewConfig::global()->dynWordWrapIndicators());
    textareaUi->sbDynamicWordWrapDepth->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
    textareaUi->chkShowTabs->setChecked(KateDocumentConfig::global()->showTabs());
    textareaUi->chkShowSpaces->setChecked(KateDocumentConfig::global()->showSpaces());
    bordersUi->chkLineNumbers->setChecked(KateViewConfig::global()->lineNumbers());
    bordersUi->chkIconBorder->setChecked(KateViewConfig::global()->iconBar());
    bordersUi->chkScrollbarMarks->setChecked(KateViewConfig::global()->scrollBarMarks());
    bordersUi->chkScrollbarMiniMap->setChecked(KateViewConfig::global()->scrollBarMiniMap());
    bordersUi->chkScrollbarMiniMapAll->setChecked(KateViewConfig::global()->scrollBarMiniMapAll());
    bordersUi->spBoxMiniMapWidth->setValue(KateViewConfig::global()->scrollBarMiniMapWidth());
    bordersUi->chkShowFoldingMarkers->setChecked(KateViewConfig::global()->foldingBar());
    bordersUi->chkShowLineModification->setChecked(KateViewConfig::global()->lineModification());
    bordersUi->rbSortBookmarksByPosition->setChecked(KateViewConfig::global()->bookmarkSort() == 0);
    bordersUi->rbSortBookmarksByCreation->setChecked(KateViewConfig::global()->bookmarkSort() == 1);
    bordersUi->cmbShowScrollbars->setCurrentIndex(KateViewConfig::global()->showScrollbars());
    textareaUi->chkShowIndentationLines->setChecked(KateRendererConfig::global()->showIndentationLines());
    textareaUi->chkShowWholeBracketExpression->setChecked(KateRendererConfig::global()->showWholeBracketExpression());
    textareaUi->chkAnimateBracketMatching->setChecked(KateRendererConfig::global()->animateBracketMatching());
    textareaUi->chkFoldFirstLine->setChecked(KateViewConfig::global()->foldFirstLine());
}

void KateViewDefaultsConfig::reset()
{
    ;
}

void KateViewDefaultsConfig::defaults()
{
    ;
}

QString KateViewDefaultsConfig::name() const
{
    return i18n("Appearance");
}

QString KateViewDefaultsConfig::fullName() const
{
    return i18n("Appearance");
}

QIcon KateViewDefaultsConfig::icon() const
{
    return QIcon::fromTheme(QStringLiteral("preferences-desktop-theme"));
}

//END KateViewDefaultsConfig

//BEGIN KateSaveConfigTab
KateSaveConfigTab::KateSaveConfigTab(QWidget *parent)
    : KateConfigPage(parent)
    , modeConfigPage(new ModeConfigPage(this))
{
    // FIXME: Is really needed to move all this code below to another class,
    // since it is another tab itself on the config dialog. This means we should
    // initialize, add and work with as we do with modeConfigPage (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    QTabWidget *tabWidget = new QTabWidget(this);

    QWidget *tmpWidget = new QWidget(tabWidget);
    QVBoxLayout *internalLayout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(tabWidget);
    ui = new Ui::OpenSaveConfigWidget();
    ui->setupUi(newWidget);

    QWidget *tmpWidget2 = new QWidget(tabWidget);
    QVBoxLayout *internalLayout2 = new QVBoxLayout;
    QWidget *newWidget2 = new QWidget(tabWidget);
    uiadv = new Ui::OpenSaveConfigAdvWidget();
    uiadv->setupUi(newWidget2);

    // What's this help is added in ui/opensaveconfigwidget.ui
    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->cmbEncoding, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbEncodingDetection, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbEncodingFallback, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbEOL, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->chkDetectEOL, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkEnableBOM, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->lineLengthLimit, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->cbRemoveTrailingSpaces, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkNewLineAtEof, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(uiadv->chkBackupLocalFiles, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(uiadv->chkBackupRemoteFiles, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(uiadv->edtBackupPrefix, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(uiadv->edtBackupSuffix, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(uiadv->cmbSwapFileMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
    connect(uiadv->cmbSwapFileMode, SIGNAL(currentIndexChanged(int)), this, SLOT(swapFileModeChanged(int)));
    connect(uiadv->kurlSwapDirectory, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(uiadv->spbSwapFileSync, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

    internalLayout->addWidget(newWidget);
    tmpWidget->setLayout(internalLayout);
    internalLayout2->addWidget(newWidget2);
    tmpWidget2->setLayout(internalLayout2);

    // add all tabs
    tabWidget->insertTab(0, tmpWidget, i18n("General"));
    tabWidget->insertTab(1, tmpWidget2, i18n("Advanced"));
    tabWidget->insertTab(2, modeConfigPage, modeConfigPage->name());

    connect(modeConfigPage, SIGNAL(changed()), this, SLOT(slotChanged()));

    layout->addWidget(tabWidget);
    setLayout(layout);
}

KateSaveConfigTab::~KateSaveConfigTab()
{
    delete ui;
}

void KateSaveConfigTab::swapFileModeChanged(int idx)
{
    const KateDocumentConfig::SwapFileMode mode = static_cast<KateDocumentConfig::SwapFileMode>(idx);
    switch (mode) {
    case KateDocumentConfig::DisableSwapFile:
        uiadv->lblSwapDirectory->setEnabled(false);
        uiadv->kurlSwapDirectory->setEnabled(false);
        uiadv->lblSwapFileSync->setEnabled(false);
        uiadv->spbSwapFileSync->setEnabled(false);
        break;
    case KateDocumentConfig::EnableSwapFile:
        uiadv->lblSwapDirectory->setEnabled(false);
        uiadv->kurlSwapDirectory->setEnabled(false);
        uiadv->lblSwapFileSync->setEnabled(true);
        uiadv->spbSwapFileSync->setEnabled(true);
        break;
    case KateDocumentConfig::SwapFilePresetDirectory:
        uiadv->lblSwapDirectory->setEnabled(true);
        uiadv->kurlSwapDirectory->setEnabled(true);
        uiadv->lblSwapFileSync->setEnabled(true);
        uiadv->spbSwapFileSync->setEnabled(true);
        break;
    }
}

void KateSaveConfigTab::apply()
{
    modeConfigPage->apply();

    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateGlobalConfig::global()->configStart();
    KateDocumentConfig::global()->configStart();

    if (uiadv->edtBackupSuffix->text().isEmpty() && uiadv->edtBackupPrefix->text().isEmpty()) {
        KMessageBox::information(
            this,
            i18n("You did not provide a backup suffix or prefix. Using default suffix: '~'"),
            i18n("No Backup Suffix or Prefix")
        );
        uiadv->edtBackupSuffix->setText(QStringLiteral("~"));
    }

    const KateDocumentConfig::SwapFileMode swap_mode = static_cast<KateDocumentConfig::SwapFileMode>(uiadv->cmbSwapFileMode->currentIndex());
    if (swap_mode == KateDocumentConfig::SwapFilePresetDirectory) {
        const QString dirpath = uiadv->kurlSwapDirectory->url().toLocalFile();

        if (!QDir(dirpath).exists()) {
            KMessageBox::ButtonCode res = KMessageBox::warningYesNo(this,
                                          i18n("Selected directory for swap file storage does not exist. Do you want to create it?"),
                                          i18n("Missing Swap File Directory"));

            if (res == KMessageBox::Yes) {
                QDir().mkpath(dirpath);
            } else {
                // FIXME: what to do here? Return to config?
            }
        }

        if (!QFileInfo(dirpath).isDir()) {
            //FIXME: we are screwed here, revert to  KateDocumentConfig::EnableSwapFile?
        }

    }

    uint f(0);
    if (uiadv->chkBackupLocalFiles->isChecked()) {
        f |= KateDocumentConfig::LocalFiles;
    }
    if (uiadv->chkBackupRemoteFiles->isChecked()) {
        f |= KateDocumentConfig::RemoteFiles;
    }

    KateDocumentConfig::global()->setBackupFlags(f);
    KateDocumentConfig::global()->setBackupPrefix(uiadv->edtBackupPrefix->text());
    KateDocumentConfig::global()->setBackupSuffix(uiadv->edtBackupSuffix->text());

    KateDocumentConfig::global()->setSwapFileMode(uiadv->cmbSwapFileMode->currentIndex());
    KateDocumentConfig::global()->setSwapDirectory(uiadv->kurlSwapDirectory->url().toLocalFile());
    KateDocumentConfig::global()->setSwapSyncInterval(uiadv->spbSwapFileSync->value());

    KateDocumentConfig::global()->setRemoveSpaces(ui->cbRemoveTrailingSpaces->currentIndex());

    KateDocumentConfig::global()->setNewLineAtEof(ui->chkNewLineAtEof->isChecked());

    // set both standard and fallback encoding
    KateDocumentConfig::global()->setEncoding((ui->cmbEncoding->currentIndex() == 0) ? QString() : KCharsets::charsets()->encodingForName(ui->cmbEncoding->currentText()));

    KateGlobalConfig::global()->setProberType((KEncodingProber::ProberType)ui->cmbEncodingDetection->currentIndex());
    KateGlobalConfig::global()->setFallbackEncoding(KCharsets::charsets()->encodingForName(ui->cmbEncodingFallback->currentText()));

    KateDocumentConfig::global()->setEol(ui->cmbEOL->currentIndex());
    KateDocumentConfig::global()->setAllowEolDetection(ui->chkDetectEOL->isChecked());
    KateDocumentConfig::global()->setBom(ui->chkEnableBOM->isChecked());

    KateDocumentConfig::global()->setLineLengthLimit(ui->lineLengthLimit->value());

    KateDocumentConfig::global()->configEnd();
    KateGlobalConfig::global()->configEnd();
}

void KateSaveConfigTab::reload()
{
    modeConfigPage->reload();

    // encodings
    ui->cmbEncoding->clear();
    ui->cmbEncoding->addItem(i18n("KDE Default"));
    ui->cmbEncoding->setCurrentIndex(0);
    ui->cmbEncodingFallback->clear();
    QStringList encodings(KCharsets::charsets()->descriptiveEncodingNames());
    int insert = 1;
    for (int i = 0; i < encodings.count(); i++) {
        bool found = false;
        QTextCodec *codecForEnc = KCharsets::charsets()->codecForName(KCharsets::charsets()->encodingForName(encodings[i]), found);

        if (found) {
            ui->cmbEncoding->addItem(encodings[i]);
            ui->cmbEncodingFallback->addItem(encodings[i]);

            if (codecForEnc->name() == KateDocumentConfig::global()->encoding().toLatin1()) {
                ui->cmbEncoding->setCurrentIndex(insert);
            }

            if (codecForEnc == KateGlobalConfig::global()->fallbackCodec()) {
                // adjust index for fallback config, has no default!
                ui->cmbEncodingFallback->setCurrentIndex(insert - 1);
            }

            insert++;
        }
    }

    // encoding detection
    ui->cmbEncodingDetection->clear();
    bool found = false;
    for (int i = 0; !KEncodingProber::nameForProberType((KEncodingProber::ProberType) i).isEmpty(); ++i) {
        ui->cmbEncodingDetection->addItem(KEncodingProber::nameForProberType((KEncodingProber::ProberType) i));
        if (i == KateGlobalConfig::global()->proberType()) {
            ui->cmbEncodingDetection->setCurrentIndex(ui->cmbEncodingDetection->count() - 1);
            found = true;
        }
    }
    if (!found) {
        ui->cmbEncodingDetection->setCurrentIndex(KEncodingProber::Universal);
    }

    // eol
    ui->cmbEOL->setCurrentIndex(KateDocumentConfig::global()->eol());
    ui->chkDetectEOL->setChecked(KateDocumentConfig::global()->allowEolDetection());
    ui->chkEnableBOM->setChecked(KateDocumentConfig::global()->bom());
    ui->lineLengthLimit->setValue(KateDocumentConfig::global()->lineLengthLimit());

    ui->cbRemoveTrailingSpaces->setCurrentIndex(KateDocumentConfig::global()->removeSpaces());
    ui->chkNewLineAtEof->setChecked(KateDocumentConfig::global()->newLineAtEof());

    // other stuff
    uint f(KateDocumentConfig::global()->backupFlags());
    uiadv->chkBackupLocalFiles->setChecked(f & KateDocumentConfig::LocalFiles);
    uiadv->chkBackupRemoteFiles->setChecked(f & KateDocumentConfig::RemoteFiles);
    uiadv->edtBackupPrefix->setText(KateDocumentConfig::global()->backupPrefix());
    uiadv->edtBackupSuffix->setText(KateDocumentConfig::global()->backupSuffix());
    uiadv->cmbSwapFileMode->setCurrentIndex(KateDocumentConfig::global()->swapFileModeRaw());
    uiadv->kurlSwapDirectory->setUrl(QUrl::fromLocalFile(KateDocumentConfig::global()->swapDirectory()));
    uiadv->spbSwapFileSync->setValue(KateDocumentConfig::global()->swapSyncInterval());
    swapFileModeChanged(KateDocumentConfig::global()->swapFileMode());
}

void KateSaveConfigTab::reset()
{
    modeConfigPage->reset();
}

void KateSaveConfigTab::defaults()
{
    modeConfigPage->defaults();

    ui->cbRemoveTrailingSpaces->setCurrentIndex(0);

    uiadv->chkBackupLocalFiles->setChecked(true);
    uiadv->chkBackupRemoteFiles->setChecked(false);
    uiadv->edtBackupPrefix->setText(QString());
    uiadv->edtBackupSuffix->setText(QStringLiteral("~"));
    uiadv->cmbSwapFileMode->setCurrentIndex(1);
    uiadv->kurlSwapDirectory->setDisabled(true);
    uiadv->lblSwapDirectory->setDisabled(true);
    uiadv->spbSwapFileSync->setValue(15);
}

QString KateSaveConfigTab::name() const
{
    return i18n("Open/Save");
}

QString KateSaveConfigTab::fullName() const
{
    return i18n("File Opening & Saving");
}

QIcon KateSaveConfigTab::icon() const
{
    return QIcon::fromTheme(QStringLiteral("document-save"));
}

//END KateSaveConfigTab

//BEGIN KateHlDownloadDialog
KateHlDownloadDialog::KateHlDownloadDialog(QWidget *parent, const char *name, bool modal)
    : QDialog(parent)
{
    setWindowTitle(i18n("Highlight Download"));
    setObjectName(QString::fromUtf8(name));
    setModal(modal);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QLabel *label = new QLabel(i18n("Select the syntax highlighting files you want to update:"), this);
    mainLayout->addWidget(label);

    list = new QTreeWidget(this);
    list->setColumnCount(4);
    list->setHeaderLabels(QStringList() << QString() << i18n("Name") << i18n("Installed") << i18n("Latest"));
    list->setSelectionMode(QAbstractItemView::MultiSelection);
    list->setAllColumnsShowFocus(true);
    list->setRootIsDecorated(false);
    list->setColumnWidth(0, 22);
    mainLayout->addWidget(list);

    label = new QLabel(i18n("<b>Note:</b> New versions are selected automatically."), this);
    mainLayout->addWidget(label);

    // buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    mainLayout->addWidget(buttons);

    m_installButton = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-ok")), i18n("&Install"));
    m_installButton->setDefault(true);
    buttons->addButton(m_installButton, QDialogButtonBox::AcceptRole);
    connect(m_installButton, SIGNAL(clicked()), this, SLOT(slotInstall()));

    QPushButton *closeButton = new QPushButton;
    KGuiItem::assign(closeButton, KStandardGuiItem::cancel());
    buttons->addButton(closeButton, QDialogButtonBox::RejectRole);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    transferJob = KIO::get(QUrl(QStringLiteral("%1update-%2.%3.xml").arg(HLDOWNLOADPATH).arg(KTEXTEDITOR_VERSION_MAJOR).arg(KTEXTEDITOR_VERSION_MINOR)), KIO::Reload);
    connect(transferJob, SIGNAL(data(KIO::Job*,QByteArray)),
            this, SLOT(listDataReceived(KIO::Job*,QByteArray)));
//        void data( KIO::Job *, const QByteArray &data);

    resize(450, 400);
}

KateHlDownloadDialog::~KateHlDownloadDialog() {}

/// Split typical version string (\c major.minor.patch) into
/// numeric components, convert 'em to \c unsigned and form a
/// single value that can be compared w/ other versions
/// using relation operators.
/// \note It takes into account only first 3 numbers
unsigned KateHlDownloadDialog::parseVersion(const QString &version_string)
{
    unsigned vn[3] = {0, 0, 0};
    unsigned idx = 0;
    foreach (const QString &n, version_string.split(QLatin1Char('.'))) {
        vn[idx++] = n.toUInt();
        if (idx == sizeof(vn)) {
            break;
        }
    }
    return (((vn[0]) << 16) | ((vn[1]) << 8) | (vn[2]));
}

void KateHlDownloadDialog::listDataReceived(KIO::Job *, const QByteArray &data)
{
    if (!transferJob || transferJob->isErrorPage()) {
        m_installButton->setEnabled(false);
        if (data.size() == 0) {
            KMessageBox::error(this, i18n("The list of highlightings could not be found on / retrieved from the server"));
        }
        return;
    }

    listData += QLatin1String(data);
    qCDebug(LOG_KTE) << QStringLiteral("CurrentListData: ") << listData;
    qCDebug(LOG_KTE) << QStringLiteral("Data length: %1").arg(data.size());
    qCDebug(LOG_KTE) << QStringLiteral("listData length: %1").arg(listData.length());
    if (data.size() == 0) {
        if (listData.length() > 0) {
            QString installedVersion;
            KateHlManager *hlm = KateHlManager::self();
            QDomDocument doc;
            doc.setContent(listData);
            QDomElement DocElem = doc.documentElement();
            QDomNode n = DocElem.firstChild();
            KateHighlighting *hl = 0;

            if (n.isNull()) {
                qCDebug(LOG_KTE) << QStringLiteral("There is no usable childnode");
            }
            while (!n.isNull()) {
                installedVersion = QStringLiteral("    --");

                QDomElement e = n.toElement();
                if (!e.isNull()) {
                    qCDebug(LOG_KTE) << QStringLiteral("NAME: ") << e.tagName() << QStringLiteral(" - ") << e.attribute(QStringLiteral("name"));
                }
                n = n.nextSibling();

                QString Name = e.attribute(QStringLiteral("name"));

                for (int i = 0; i < hlm->highlights(); i++) {
                    hl = hlm->getHl(i);
                    if (hl && hl->name() == Name) {
                        installedVersion = QLatin1String("    ") + hl->version();
                        break;
                    } else {
                        hl = 0;
                    }
                }

                // autoselect entry if new or updated.
                QTreeWidgetItem *entry = new QTreeWidgetItem(list);
                entry->setText(0, QString());
                entry->setText(1, e.attribute(QStringLiteral("name")));
                entry->setText(2, installedVersion);
                entry->setText(3, e.attribute(QStringLiteral("version")));
                entry->setText(4, e.attribute(QStringLiteral("url")));

                bool is_fresh = false;
                if (hl) {
                    unsigned prev_version = parseVersion(hl->version());
                    unsigned next_version = parseVersion(e.attribute(QStringLiteral("version")));
                    is_fresh = prev_version < next_version;
                } else {
                    is_fresh = true;
                }
                if (is_fresh) {
                    entry->treeWidget()->setItemSelected(entry, true);
                    entry->setIcon(0, QIcon::fromTheme((QStringLiteral("get-hot-new-stuff"))));
                }
            }
            list->resizeColumnToContents(1);
            list->sortItems(1, Qt::AscendingOrder);
        }
    }
}

void KateHlDownloadDialog::slotInstall()
{
    const QString destdir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/katepart5/syntax/");
    QDir(destdir).mkpath(QStringLiteral(".")); // make sure the dir is there

    foreach (QTreeWidgetItem *it, list->selectedItems()) {
        QUrl src(it->text(4));
        QString filename = src.fileName();

        // if there is no fileName construct at least something
        if (filename.isEmpty()) {
            filename = src.path().replace(QLatin1Char('/'), QLatin1Char('_'));
        }

        QUrl dest = QUrl::fromLocalFile(destdir + filename);

        KIO::FileCopyJob *job = KIO::file_copy(src, dest);
        KJobWidgets::setWindow(job, this);
        job->exec();
    }
}
//END KateHlDownloadDialog

//BEGIN KateGotoBar
KateGotoBar::KateGotoBar(KTextEditor::View *view, QWidget *parent)
    : KateViewBarWidget(true, parent)
    , m_view(view)
{
    Q_ASSERT(m_view != 0);    // this bar widget is pointless w/o a view

    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setMargin(0);
    gotoRange = new QSpinBox(centralWidget());

    QLabel *label = new QLabel(i18n("&Go to line:"), centralWidget());
    label->setBuddy(gotoRange);

    QToolButton *btnOK = new QToolButton(centralWidget());
    btnOK->setAutoRaise(true);
    btnOK->setIcon(QIcon::fromTheme(QStringLiteral("go-jump")));
    btnOK->setText(i18n("Go"));
    btnOK->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(btnOK, SIGNAL(clicked()), this, SLOT(gotoLine()));

    topLayout->addWidget(label);
    topLayout->addWidget(gotoRange, 1);
    topLayout->setStretchFactor(gotoRange, 0);
    topLayout->addWidget(btnOK);
    topLayout->addStretch();

    setFocusProxy(gotoRange);
}

void KateGotoBar::updateData()
{
    gotoRange->setMaximum(m_view->document()->lines());
    if (!isVisible()) {
        gotoRange->setValue(m_view->cursorPosition().line() + 1);
        gotoRange->adjustSize(); // ### does not respect the range :-(
    }
    gotoRange->setFocus(Qt::OtherFocusReason);
    gotoRange->selectAll();
}

void KateGotoBar::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        gotoLine();
        return;
    }
    KateViewBarWidget::keyPressEvent(event);
}

void KateGotoBar::gotoLine()
{
    KTextEditor::ViewPrivate *kv = qobject_cast<KTextEditor::ViewPrivate *>(m_view);
    if (kv && kv->selection() && !kv->config()->persistentSelection()) {
        kv->clearSelection();
    }

    m_view->setCursorPosition(KTextEditor::Cursor(gotoRange->value() - 1, 0));
    m_view->setFocus();
    emit hideMe();
}
//END KateGotoBar

//BEGIN KateDictionaryBar
KateDictionaryBar::KateDictionaryBar(KTextEditor::ViewPrivate *view, QWidget *parent)
    : KateViewBarWidget(true, parent)
    , m_view(view)
{
    Q_ASSERT(m_view != 0); // this bar widget is pointless w/o a view

    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setMargin(0);
    //topLayout->setSpacing(spacingHint());
    m_dictionaryComboBox = new Sonnet::DictionaryComboBox(centralWidget());
    connect(m_dictionaryComboBox, SIGNAL(dictionaryChanged(QString)),
            this, SLOT(dictionaryChanged(QString)));
    connect(view->doc(), SIGNAL(defaultDictionaryChanged(KTextEditor::DocumentPrivate*)),
            this, SLOT(updateData()));
    QLabel *label = new QLabel(i18n("Dictionary:"), centralWidget());
    label->setBuddy(m_dictionaryComboBox);

    topLayout->addWidget(label);
    topLayout->addWidget(m_dictionaryComboBox, 1);
    topLayout->setStretchFactor(m_dictionaryComboBox, 0);
    topLayout->addStretch();
}

KateDictionaryBar::~KateDictionaryBar()
{
}

void KateDictionaryBar::updateData()
{
    KTextEditor::DocumentPrivate *document = m_view->doc();
    QString dictionary = document->defaultDictionary();
    if (dictionary.isEmpty()) {
        dictionary = Sonnet::Speller().defaultLanguage();
    }
    m_dictionaryComboBox->setCurrentByDictionary(dictionary);
}

void KateDictionaryBar::dictionaryChanged(const QString &dictionary)
{
    KTextEditor::Range selection = m_view->selectionRange();
    if (selection.isValid() && !selection.isEmpty()) {
        m_view->doc()->setDictionary(dictionary, selection);
    } else {
        m_view->doc()->setDefaultDictionary(dictionary);
    }
}

//END KateGotoBar

//BEGIN KateModOnHdPrompt
KateModOnHdPrompt::KateModOnHdPrompt(KTextEditor::DocumentPrivate *doc,
                                     KTextEditor::ModificationInterface::ModifiedOnDiskReason modtype,
                                     const QString &reason,
                                     QWidget *parent)
    : QDialog(parent),
      m_doc(doc),
      m_modtype(modtype),
      m_proc(0),
      m_diffFile(0)
{
    QString title, okText, okIcon, okToolTip;
    if (modtype == KTextEditor::ModificationInterface::OnDiskDeleted) {
        title = i18n("File Was Deleted on Disk");
        okText = i18n("&Save File As...");
        okIcon = QStringLiteral("document-save-as");
        okToolTip = i18n("Lets you select a location and save the file again.");
    } else {
        title = i18n("File Changed on Disk");
        okText = i18n("&Reload File");
        okIcon = QStringLiteral("view-refresh");
        okToolTip = i18n("Reload the file from disk. If you have unsaved changes, "
                         "they will be lost.");
    }

    setWindowTitle(title);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QWidget *w = new QWidget(this);
    mainLayout->addWidget(w);
    ui = new Ui::ModOnHdWidget();
    ui->setupUi(w);
    ui->lblIcon->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(IconSize(KIconLoader::Desktop), IconSize(KIconLoader::Desktop)));
    ui->lblText->setText(reason + QLatin1String("\n\n") + i18n("What do you want to do?"));

    // buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    mainLayout->addWidget(buttons);

    QPushButton *okButton = new QPushButton(QIcon::fromTheme(okIcon), okText);
    okButton->setToolTip(okToolTip);
    buttons->addButton(okButton, QDialogButtonBox::AcceptRole);
    connect(okButton, SIGNAL(clicked()), this, SLOT(slotOk()));

    QPushButton *applyButton = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-warning")), i18n("&Ignore Changes"));
    applyButton->setToolTip(i18n("Ignore the changes. You will not be prompted again."));
    buttons->addButton(applyButton, QDialogButtonBox::ApplyRole);
    connect(applyButton, SIGNAL(clicked()), this, SLOT(slotApply()));

    QPushButton *cancelButton = new QPushButton;
    KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
    cancelButton->setToolTip(i18n("Do nothing. Next time you focus the file, "
                                  "or try to save it or close it, you will be prompted again."));
    buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    // If the file isn't deleted, present a diff button, and a overwrite action.
    if (modtype != KTextEditor::ModificationInterface::OnDiskDeleted) {
        QPushButton *overwriteButton = new QPushButton;
        KGuiItem::assign(overwriteButton, KStandardGuiItem::overwrite());
        overwriteButton->setToolTip(i18n("Overwrite the disk file with the editor content."));
        buttons->addButton(overwriteButton, QDialogButtonBox::ActionRole);
        connect(overwriteButton, SIGNAL(clicked()), this, SLOT(slotOverwrite()));

        connect(ui->btnDiff, SIGNAL(clicked()), this, SLOT(slotDiff()));
    } else {
        ui->chkIgnoreWhiteSpaces->setVisible(false);
        ui->btnDiff->setVisible(false);

        QPushButton *closeButton = new QPushButton;
        KGuiItem::assign(closeButton, KStandardGuiItem::close());
        closeButton->setToolTip(i18n("Close the document."));
        buttons->addButton(closeButton, QDialogButtonBox::DestructiveRole);
        connect(closeButton, SIGNAL(clicked()), this, SLOT(slotClose()));
    }
}

KateModOnHdPrompt::~KateModOnHdPrompt()
{
    delete m_proc;
    m_proc = 0;
    if (m_diffFile) {
        m_diffFile->setAutoRemove(true);
        delete m_diffFile;
        m_diffFile = 0;
    }
    delete ui;
}

void KateModOnHdPrompt::slotDiff()
{
    if (m_diffFile) {
        return;
    }

    m_diffFile = new QTemporaryFile();
    m_diffFile->open();

    // Start a KProcess that creates a diff
    m_proc = new KProcess(this);
    m_proc->setOutputChannelMode(KProcess::MergedChannels);
    *m_proc << QStringLiteral("diff") << QString(ui->chkIgnoreWhiteSpaces->isChecked() ? QLatin1String("-ub") : QLatin1String("-u"))
            << QStringLiteral("-") <<  m_doc->url().toLocalFile();
    connect(m_proc, SIGNAL(readyRead()), this, SLOT(slotDataAvailable()));
    connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotPDone()));

    setCursor(Qt::WaitCursor);
    // disable the button and checkbox, to hinder the user to run it twice.
    ui->chkIgnoreWhiteSpaces->setEnabled(false);
    ui->btnDiff->setEnabled(false);

    m_proc->start();

    QTextStream ts(m_proc);
    int lastln = m_doc->lines() - 1;
    for (int l = 0; l < lastln; ++l) {
        ts << m_doc->line(l) << '\n';
    }
    ts << m_doc->line(lastln);
    ts.flush();
    m_proc->closeWriteChannel();
}

void KateModOnHdPrompt::slotDataAvailable()
{
    m_diffFile->write(m_proc->readAll());
}

void KateModOnHdPrompt::slotPDone()
{
    setCursor(Qt::ArrowCursor);
    ui->chkIgnoreWhiteSpaces->setEnabled(true);
    ui->btnDiff->setEnabled(true);

    const QProcess::ExitStatus es = m_proc->exitStatus();
    delete m_proc;
    m_proc = 0;

    if (es != QProcess::NormalExit) {
        KMessageBox::sorry(this,
                           i18n("The diff command failed. Please make sure that "
                                "diff(1) is installed and in your PATH."),
                           i18n("Error Creating Diff"));
        delete m_diffFile;
        m_diffFile = 0;
        return;
    }

    if (m_diffFile->size() == 0) {
        if (ui->chkIgnoreWhiteSpaces->isChecked()) {
            KMessageBox::information(this,
                                     i18n("The files are identical."),
                                     i18n("Diff Output"));
        } else {
            KMessageBox::information(this,
                                     i18n("Ignoring amount of white space changed, the files are identical."),
                                     i18n("Diff Output"));
        }
        delete m_diffFile;
        m_diffFile = 0;
        return;
    }

    m_diffFile->setAutoRemove(false);
    QUrl url = QUrl::fromLocalFile(m_diffFile->fileName());
    delete m_diffFile;
    m_diffFile = 0;

    // KRun::runUrl should delete the file, once the client exits
    KRun::runUrl(url, QStringLiteral("text/x-patch"), this, true);
}

void KateModOnHdPrompt::slotOk()
{
    done((m_modtype == KTextEditor::ModificationInterface::OnDiskDeleted) ? Save : Reload);
}

void KateModOnHdPrompt::slotApply()
{
    if (KMessageBox::warningContinueCancel(this,
                                           i18n("Ignoring means that you will not be warned again (unless "
                                                   "the disk file changes once more): if you save the document, you "
                                                   "will overwrite the file on disk; if you do not save then the disk "
                                                   "file (if present) is what you have."),
                                           i18n("You Are on Your Own"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           QStringLiteral("kate_ignore_modonhd")) != KMessageBox::Continue) {
        return;
    }

    done(Ignore);
}

void KateModOnHdPrompt::slotOverwrite()
{
    done(Overwrite);
}

void KateModOnHdPrompt::slotClose()
{
    done(Close);
}

//END KateModOnHdPrompt

