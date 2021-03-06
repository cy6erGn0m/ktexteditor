/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef __KATE_GLOBAL_H__
#define __KATE_GLOBAL_H__

#include <ktexteditor_export.h>
#include "katescript.h"

#include <ktexteditor/editor.h>
#include "ktexteditor/view.h"

#include <KAboutData>
#include <KSharedConfig>

#include <KTextEditor/Application>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Command>

#include <QList>
#include <QPointer>
#include <QMap>
#include <QScopedPointer>

class QStringListModel;

class KateCmd;
class KateModeManager;
class KateSchemaManager;
class KateGlobalConfig;
class KateDocumentConfig;
class KateViewConfig;
class KateRendererConfig;
namespace KTextEditor { class DocumentPrivate; }
namespace KTextEditor { class ViewPrivate; }
class KateScriptManager;
class KDirWatch;
class KateHlManager;
class KateSpellCheckManager;
class KateWordCompletionModel;
class KateAbstractInputModeFactory;
class KateKeywordCompletionModel;
class KateDefaultColors;

namespace KTextEditor
{

/**
 * KTextEditor::EditorPrivate
 * One instance of this class is hold alive during
 * a kate part session, as long as any factory, document
 * or view stay around, here is the place to put things
 * which are needed and shared by all this objects ;)
 */
class KTEXTEDITOR_EXPORT EditorPrivate : public KTextEditor::Editor
{
    Q_OBJECT

    friend class KTextEditor::Editor;

    // unit testing support
public:
    /**
     * Calling this function internally sets a flag such that unitTestMode()
     * returns \p true.
     */
    static void enableUnitTestMode();

    /**
     * Returns \p true, if the unit test mode was enabled through a call of
     * enableUnitTestMode(), otherwise \p false.
     */
    static bool unitTestMode();

    // for setDefaultEncoding
    friend class KateDocumentConfig;

private:
    /**
     * Default constructor, private, as singleton
     * @param staticInstance pointer to fill with content of this
     */
    EditorPrivate(QPointer<KTextEditor::EditorPrivate> &staticInstance);

public:
    /**
     * Destructor
     */
    ~EditorPrivate();

    /**
     * Create a new document object
     * @param parent parent object
     * @return created KTextEditor::Document
     */
    KTextEditor::Document *createDocument(QObject *parent) Q_DECL_OVERRIDE;

    /**
     * Returns a list of all documents of this editor.
     * @return list of all existing documents
     */
    QList<KTextEditor::Document *> documents() Q_DECL_OVERRIDE
    {
        return m_documents.keys();
    }

    /**
     * Set the global application object.
     * This will allow the editor component to access
     * the hosting application.
     * @param application application object
     */
    void setApplication(KTextEditor::Application *application) Q_DECL_OVERRIDE
    {
        // switch back to dummy application?
        m_application = application ? application : &m_dummyApplication;
    }

    /**
     * Current hosting application, if any set.
     * @return current application object or nullptr
     */
    KTextEditor::Application *application() const Q_DECL_OVERRIDE
    {
        return m_application;
    }

    /**
     * General Information about this editor
     */
public:
    /**
     * return the about data
     * @return about data of this editor part
     */
    const KAboutData &aboutData() const Q_DECL_OVERRIDE
    {
        return m_aboutData;
    }

    /**
    * Configuration management
    */
public:
    /**
     * Shows a config dialog for the part, changes will be applied
     * to the editor, but not saved anywhere automagically, call
     * writeConfig to save them
    */
    void configDialog(QWidget *parent) Q_DECL_OVERRIDE;

    /**
     * Number of available config pages
     * If the editor returns a number < 1, it doesn't support this
     * and the embedding app should use the configDialog () instead
     * @return number of config pages
     */
    int configPages() const Q_DECL_OVERRIDE;

    /**
     * returns config page with the given number,
     * config pages from 0 to configPages()-1 are available
     * if configPages() > 0
     */
    KTextEditor::ConfigPage *configPage(int number, QWidget *parent) Q_DECL_OVERRIDE;

    /**
     * Kate Part Internal stuff ;)
     */
public:
    /**
     * singleton accessor
     * @return instance of the factory
     */
    static KTextEditor::EditorPrivate *self();

    /**
     * register document at the factory
     * this allows us to loop over all docs for example on config changes
     * @param doc document to register
     */
    void registerDocument(KTextEditor::DocumentPrivate *doc);

    /**
     * unregister document at the factory
     * @param doc document to register
     */
    void deregisterDocument(KTextEditor::DocumentPrivate *doc);

    /**
     * register view at the factory
     * this allows us to loop over all views for example on config changes
     * @param view view to register
     */
    void registerView(KTextEditor::ViewPrivate *view);

    /**
     * unregister view at the factory
     * @param view view to unregister
     */
    void deregisterView(KTextEditor::ViewPrivate *view);

    /**
     * return a list of all registered views
     * @return all known views
     */
    QList<KTextEditor::ViewPrivate *> views()
    {
        return m_views.toList();
    }

    /**
     * global dirwatch
     * @return dirwatch instance
     */
    KDirWatch *dirWatch()
    {
        return m_dirWatch;
    }

    /**
     * The global configuration of katepart, e.g. katepartrc
     * @return global shared access to katepartrc config
     */
    static KSharedConfigPtr config()
    {
        // use dummy config for unit tests!
        return KTextEditor::EditorPrivate::unitTestMode()
            ? KSharedConfig::openConfig(QStringLiteral("katepartrc-unittest"), KConfig::SimpleConfig, QStandardPaths::TempLocation)
            : KSharedConfig::openConfig(QStringLiteral("katepartrc"));
    }

    /**
     * global mode manager
     * used to manage the modes centrally
     * @return mode manager
     */
    KateModeManager *modeManager()
    {
        return m_modeManager;
    }

    /**
     * manager for the katepart schemas
     * @return schema manager
     */
    KateSchemaManager *schemaManager()
    {
        return m_schemaManager;
    }

    /**
     * fallback document config
     * @return default config for all documents
     */
    KateDocumentConfig *documentConfig()
    {
        return m_documentConfig;
    }

    /**
     * fallback view config
     * @return default config for all views
     */
    KateViewConfig *viewConfig()
    {
        return m_viewConfig;
    }

    /**
     * fallback renderer config
     * @return default config for all renderers
     */
    KateRendererConfig *rendererConfig()
    {
        return m_rendererConfig;
    }

    /**
     * Global script collection
     */
    KateScriptManager *scriptManager()
    {
        return m_scriptManager;
    }

    /**
     * hl manager
     * @return hl manager
     */
    KateHlManager *hlManager()
    {
        return m_hlManager;
    }

    /**
     * command manager
     * @return command manager
     */
    KateCmd *cmdManager()
    {
        return m_cmdManager;
    }

    /**
     * spell check manager
     * @return spell check manager
     */
    KateSpellCheckManager *spellCheckManager()
    {
        return m_spellCheckManager;
    }

    /**
     * global instance of the simple word completion mode
     * @return global instance of the simple word completion mode
     */
    KateWordCompletionModel *wordCompletionModel()
    {
        return m_wordCompletionModel;
    }

    /**
     * Global instance of the language-aware keyword completion model
     * @return global instance of the keyword completion model
     */
    KateKeywordCompletionModel *keywordCompletionModel () {
        return m_keywordCompletionModel;
    }

    /**
     * query for command
     * @param cmd name of command to query for
     * @return found command or 0
     */
    KTextEditor::Command *queryCommand(const QString &cmd) const Q_DECL_OVERRIDE;

    /**
     * Get a list of all registered commands.
     * \return list of all commands
     */
    QList<KTextEditor::Command *> commands() const Q_DECL_OVERRIDE;

    /**
     * Get a list of available commandline strings.
     * \return commandline strings
     */
    QStringList commandList() const Q_DECL_OVERRIDE;

    /**
     * Copy text to clipboard an remember it in the history
     * @param text text to copy to clipboard, does nothing if empty!
     */
    void copyToClipboard(const QString &text);

    /**
     * Clipboard history, filled with text we ever copied
     * to clipboard via copyToClipboard.
     */
    const QStringList &clipboardHistory() const
    {
        return m_clipboardHistory;
    }

    /**
     * return a list of all registered docs
     * @return all known documents
     */
    QList<KTextEditor::DocumentPrivate *> kateDocuments()
    {
        return m_documents.values();
    }

    /**
     * Dummy main window to be null safe.
     * @return dummy main window
     */
    KTextEditor::MainWindow *dummyMainWindow()
    {
        return &m_dummyMainWindow;
    }

    /**
     * @return list of available input mode factories
     */
    QList<KateAbstractInputModeFactory *> inputModeFactories();

    /**
     * Default colors, once constructed, as expensive
     * @return default colors
     */
    const KateDefaultColors &defaultColors() const
    {
        return *m_defaultColors;
    }

    /**
     * Search pattern history shared among simple/power search instances.
     */
    QStringListModel *searchHistoryModel();

    /**
     * Replace pattern history shared among simple/power search instances.
     */
    QStringListModel *replaceHistoryModel();

    /**
     * Call this function to store the history models to the application config.
     */
    void saveSearchReplaceHistoryModels();

Q_SIGNALS:
    /**
     * Emitted if the history of clipboard changes via copyToClipboard
     */
    void clipboardHistoryChanged();

protected:
    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void updateColorPalette();

private:
    /**
     * about data (authors and more)
     */
    KAboutData m_aboutData;

    /**
     * registered docs, map from general to specialized pointer
     */
    QHash<KTextEditor::Document *, KTextEditor::DocumentPrivate *> m_documents;

    /**
     * registered views
     */
    QSet<KTextEditor::ViewPrivate *> m_views;

    /**
     * global dirwatch object
     */
    KDirWatch *m_dirWatch;

    /**
     * mode manager
     */
    KateModeManager *m_modeManager;

    /**
     * schema manager
     */
    KateSchemaManager *m_schemaManager;

    /**
     * global config
     */
    KateGlobalConfig *m_globalConfig;

    /**
     * fallback document config
     */
    KateDocumentConfig *m_documentConfig;

    /**
     * fallback view config
     */
    KateViewConfig *m_viewConfig;

    /**
     * fallback renderer config
     */
    KateRendererConfig *m_rendererConfig;

    /**
     * internal commands
     */
    QList<KTextEditor::Command *> m_cmds;

    /**
     * script manager
     */
    KateScriptManager *m_scriptManager;

    /**
     * hl manager
     */
    KateHlManager *m_hlManager;

    /**
     * command manager
     */
    KateCmd *m_cmdManager;

    /**
     * spell check manager
     */
    KateSpellCheckManager *m_spellCheckManager;

    /**
     * global instance of the simple word completion mode
     */
    KateWordCompletionModel *m_wordCompletionModel;

    /**
     * global instance of the language-specific keyword completion model
     */
    KateKeywordCompletionModel *m_keywordCompletionModel;

    /**
     * clipboard history
     */
    QStringList m_clipboardHistory;

    /**
     * Dummy application object to be null safe
     */
    KTextEditor::Application m_dummyApplication;

    /**
     * access to application
     */
    QPointer<KTextEditor::Application> m_application;

    /**
     * Dummy main window to be null safe
     */
    KTextEditor::MainWindow m_dummyMainWindow;

    /**
     * input modes map
     */
    QMap<KTextEditor::View::InputMode, KateAbstractInputModeFactory *> m_inputModeFactories;

    /**
     * default colors
     */
    QScopedPointer<KateDefaultColors> m_defaultColors;

    /**
     * Shared history models for search & replace.
     */
    QStringListModel *m_searchHistoryModel;
    QStringListModel *m_replaceHistoryModel;
};

}

#endif

