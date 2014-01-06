/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2000 Scott Manson <sdmanson@alltel.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesyntaxdocument.h"

#include "katepartdebug.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <KLocalizedString>
#include <KMessageBox>
#include <KConfig>
#include <KConfigGroup>

#include <QApplication>
#include <QFile>
#include <QDir>

// use this to turn on over verbose debug output...
#undef KSD_OVER_VERBOSE

KateSyntaxDocument::KateSyntaxDocument(KConfig *config, bool force)
  : QDomDocument()
  , m_config (config)
{
  // Let's build the Mode List (katesyntaxhighlightingrc)
  setupModeList(force);
}

KateSyntaxDocument::~KateSyntaxDocument()
{
  for (int i=0; i < myModeList.size(); i++)
    delete myModeList[i];
}

/** If the open hl file is different from the one needed, it opens
    the new one and assign some other things.
    identifier = File name and path of the new xml needed
*/
bool KateSyntaxDocument::setIdentifier(const QString& identifier)
{
  // if the current file is the same as the new one don't do anything.
  if(currentFile != identifier)
  {
    // let's open the new file
    QFile f( identifier );

    if ( f.open(QIODevice::ReadOnly) )
    {
      // Let's parse the contets of the xml file
      /* The result of this function should be check for robustness,
         a false returned means a parse error */
      QString errorMsg;
      int line, col;
      bool success=setContent(&f,&errorMsg,&line,&col);

      // Ok, now the current file is the pretended one (identifier)
      currentFile = identifier;

      // Close the file, is not longer needed
      f.close();

      if (!success)
      {
        KMessageBox::error(QApplication::activeWindow(),i18n("<qt>The error <b>%4</b><br /> has been detected in the file %1 at %2/%3</qt>", identifier,
             line, col, i18nc("QXml",errorMsg.toUtf8().data())));
        return false;
      }
    }
    else
    {
      // Oh o, we couldn't open the file.
      KMessageBox::error(QApplication::activeWindow(), i18n("Unable to open %1", identifier) );
      return false;
    }
  }
  return true;
}

/**
 * Jump to the next group, KateSyntaxContextData::currentGroup will point to the next group
 */
bool KateSyntaxDocument::nextGroup( KateSyntaxContextData* data)
{
  if(!data)
    return false;

  // No group yet so go to first child
  if (data->currentGroup.isNull())
  {
    // Skip over non-elements. So far non-elements are just comments
    QDomNode node = data->parent.firstChild();
    while (node.isComment())
      node = node.nextSibling();

    data->currentGroup = node.toElement();
  }
  else
  {
    // common case, iterate over siblings, skipping comments as we go
    QDomNode node = data->currentGroup.nextSibling();
    while (node.isComment())
      node = node.nextSibling();

    data->currentGroup = node.toElement();
  }

  return !data->currentGroup.isNull();
}

/**
 * Jump to the next item, KateSyntaxContextData::item will point to the next item
 */
bool KateSyntaxDocument::nextItem( KateSyntaxContextData* data)
{
  if(!data)
    return false;

  if (data->item.isNull())
  {
    QDomNode node = data->currentGroup.firstChild();
    while (node.isComment())
      node = node.nextSibling();

    data->item = node.toElement();
  }
  else
  {
    QDomNode node = data->item.nextSibling();
    while (node.isComment())
      node = node.nextSibling();

    data->item = node.toElement();
  }

  return !data->item.isNull();
}

/**
 * This function is used to fetch the atributes of the tags of the item in a KateSyntaxContextData.
 */
QString KateSyntaxDocument::groupItemData( const KateSyntaxContextData* data, const QString& name){
  if(!data)
    return QString();

  // If there's no name just return the tag name of data->item
  if ( (!data->item.isNull()) && (name.isEmpty()))
  {
    return data->item.tagName();
  }

  // if name is not empty return the value of the attribute name
  if (!data->item.isNull())
  {
    return data->item.attribute(name);
  }

  return QString();

}

QString KateSyntaxDocument::groupData( const KateSyntaxContextData* data,const QString& name)
{
  if(!data)
    return QString();

  if (!data->currentGroup.isNull())
  {
    return data->currentGroup.attribute(name);
  }
  else
  {
    return QString();
  }
}

void KateSyntaxDocument::freeGroupInfo( KateSyntaxContextData* data)
{
    delete data;
}

KateSyntaxContextData* KateSyntaxDocument::getSubItems(KateSyntaxContextData* data)
{
  KateSyntaxContextData *retval = new KateSyntaxContextData;

  if (data != 0)
  {
    retval->parent = data->currentGroup;
    retval->currentGroup = data->item;
  }

  return retval;
}

bool KateSyntaxDocument::getElement (QDomElement &element, const QString &mainGroupName, const QString &config)
{
#ifdef KSD_OVER_VERBOSE
  qCDebug(LOG_PART) << "Looking for \"" << mainGroupName << "\" -> \"" << config << "\".";
#endif

  QDomNodeList nodes = documentElement().childNodes();

  // Loop over all these child nodes looking for mainGroupName
  for (int i=0; i<nodes.count(); i++)
  {
    QDomElement elem = nodes.item(i).toElement();
    if (elem.tagName() == mainGroupName)
    {
      // Found mainGroupName ...
      QDomNodeList subNodes = elem.childNodes();

      // ... so now loop looking for config
      for (int j=0; j<subNodes.count(); j++)
      {
        QDomElement subElem = subNodes.item(j).toElement();
        if (subElem.tagName() == config)
        {
          // Found it!
          element = subElem;
          return true;
        }
      }

#ifdef KSD_OVER_VERBOSE
      qCDebug(LOG_PART) << "WARNING: \""<< config <<"\" wasn't found!";
#endif

      return false;
    }
  }

#ifdef KSD_OVER_VERBOSE
  qCDebug(LOG_PART) << "WARNING: \""<< mainGroupName <<"\" wasn't found!";
#endif

  return false;
}

/**
 * Get the KateSyntaxContextData of the QDomElement Config inside mainGroupName
 * KateSyntaxContextData::item will contain the QDomElement found
 */
KateSyntaxContextData* KateSyntaxDocument::getConfig(const QString& mainGroupName, const QString &config)
{
  QDomElement element;
  if (getElement(element, mainGroupName, config))
  {
    KateSyntaxContextData *data = new KateSyntaxContextData;
    data->item = element;
    return data;
  }
  return 0;
}

/**
 * Get the KateSyntaxContextData of the QDomElement Config inside mainGroupName
 * KateSyntaxContextData::parent will contain the QDomElement found
 */
KateSyntaxContextData* KateSyntaxDocument::getGroupInfo(const QString& mainGroupName, const QString &group)
{
  QDomElement element;
  if (getElement(element, mainGroupName, group + QLatin1Char('s')))
  {
    KateSyntaxContextData *data = new KateSyntaxContextData;
    data->parent = element;
    return data;
  }
  return 0;
}

/**
 * Returns a list with all the keywords inside the list type
 */
QStringList& KateSyntaxDocument::finddata(const QString& mainGroup, const QString& type, bool clearList)
{
#ifdef KSD_OVER_VERBOSE
  qCDebug(LOG_PART)<<"Create a list of keywords \""<<type<<"\" from \""<<mainGroup<<"\".";
#endif

  if (clearList)
    m_data.clear();

  for(QDomNode node = documentElement().firstChild(); !node.isNull(); node = node.nextSibling())
  {
    QDomElement elem = node.toElement();
    if (elem.tagName() == mainGroup)
    {
#ifdef KSD_OVER_VERBOSE
      qCDebug(LOG_PART)<<"\""<<mainGroup<<"\" found.";
#endif

      QDomNodeList nodelist1 = elem.elementsByTagName(QLatin1String("list"));

      for (int l=0; l<nodelist1.count(); l++)
      {
        if (nodelist1.item(l).toElement().attribute(QLatin1String("name")) == type)
        {
#ifdef KSD_OVER_VERBOSE
          qCDebug(LOG_PART)<<"List with attribute name=\""<<type<<"\" found.";
#endif
          
          QDomNodeList childlist = nodelist1.item(l).toElement().childNodes();

          for (int i=0; i<childlist.count(); i++)
          {
            QString element = childlist.item(i).toElement().text().trimmed();
            if (element.isEmpty())
              continue;

#ifdef KSD_OVER_VERBOSE
            if (i<6)
            {
              qCDebug(LOG_PART)<<"\""<<element<<"\" added to the list \""<<type<<"\"";
            }
            else if(i==6)
            {
              qCDebug(LOG_PART)<<"... The list continues ...";
            }
#endif

            m_data += element;
          }

          break;
        }
      }
      break;
    }
  }

  return m_data;
}

// Private
/** Generate the list of hl modes, store them in myModeList
    force: if true forces to rebuild the Mode List from the xml files (instead of katesyntax...rc)
*/
void KateSyntaxDocument::setupModeList (bool force)
{
  // If there's something in myModeList the Mode List was already built so, don't do it again
  if (!myModeList.isEmpty())
    return;

  // We'll store the ModeList in katesyntaxhighlightingrc
  KConfigGroup generalConfig(m_config, "General");

  // figure our if the kate install is too new
  if (generalConfig.readEntry ("Version",0) > generalConfig.readEntry ("CachedVersion",0))
  {
    generalConfig.writeEntry ("CachedVersion", generalConfig.readEntry ("Version",0));
    force = true;
  }

  // Let's get a list of all the xml files for hl
  QStringList list;

  const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("katepart/syntax"), QStandardPaths::LocateDirectory);
  foreach (const QString& dir, dirs) {
    const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.xml"));
    foreach (const QString& file, fileNames) {
      list.append(dir + QLatin1Char('/') + file);
    }
  }

  // Let's iterate through the list and build the Mode List
  for ( QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
  {
    // Each file has a group called:
    QString group(QLatin1String("Cache ") + *it);

    // Let's go to this group
    KConfigGroup config(m_config, group);

    const int lastModified = QFileInfo(group).lastModified().toTime_t();

    // If the group exist and we're not forced to read the xml file, let's build myModeList for katesyntax..rc
    if (!force && config.exists() && (lastModified == config.readEntry("lastModified",0)))
    {
      // Let's make a new KateSyntaxModeListItem to instert in myModeList from the information in katesyntax..rc
      KateSyntaxModeListItem *mli=new KateSyntaxModeListItem;
      mli->name       = config.readEntry("name");
      mli->nameTranslated = i18nc("Language",mli->name.toUtf8().data());
      mli->section    = i18nc("Language Section",config.readEntry("section").toUtf8().data());
      mli->mimetype   = config.readEntry("mimetype");
      mli->extension  = config.readEntry("extension");
      mli->version    = config.readEntry("version");
      mli->priority   = config.readEntry("priority");
      mli->style      = config.readEntry("style");
      mli->author    = config.readEntry("author");
      mli->license   = config.readEntry("license");
      mli->indenter = config.readEntry("indenter");
      mli->hidden   =  config.readEntry("hidden", false);
      mli->identifier = *it;

      // Apend the item to the list
      myModeList.append(mli);
    }
    else
    {
#ifdef KSD_OVER_VERBOSE
      qCDebug(LOG_PART) << "UPDATE hl cache for: " << *it;
#endif

      // We're forced to read the xml files or the mode doesn't exist in the katesyntax...rc
      QFile f(*it);

      if (f.open(QIODevice::ReadOnly))
      {
        // Ok we opened the file, let's read the contents and close the file
        /* the return of setContent should be checked because a false return shows a parsing error */
        QString errMsg;
        int line, col;

        bool success = setContent(&f,&errMsg,&line,&col);

        f.close();

        if (success)
        {
          QDomElement root = documentElement();

          if (!root.isNull())
          {
            // If the 'first' tag is language, go on
            if (root.tagName()==QLatin1String("language"))
            {
              // let's make the mode list item.
              KateSyntaxModeListItem *mli = new KateSyntaxModeListItem;

              mli->name      = root.attribute(QLatin1String("name"));
              mli->section   = root.attribute(QLatin1String("section"));
              mli->mimetype  = root.attribute(QLatin1String("mimetype"));
              mli->extension = root.attribute(QLatin1String("extensions"));
              mli->version   = root.attribute(QLatin1String("version"));
              mli->priority  = root.attribute(QLatin1String("priority"));
              mli->style     = root.attribute(QLatin1String("style"));
              mli->author    = root.attribute(QLatin1String("author"));
              mli->license   = root.attribute(QLatin1String("license"));
              mli->indenter   = root.attribute(QLatin1String("indenter"));

              QString hidden = root.attribute(QLatin1String("hidden"));
              mli->hidden    = (hidden == QLatin1String("true") || hidden == QLatin1String("TRUE"));

              mli->identifier = *it;

              // Now let's write or overwrite (if force==true) the entry in katesyntax...rc
              config = KConfigGroup(m_config, group);
              config.writeEntry("name",mli->name);
              config.writeEntry("section",mli->section);
              config.writeEntry("mimetype",mli->mimetype);
              config.writeEntry("extension",mli->extension);
              config.writeEntry("version",mli->version);
              config.writeEntry("priority",mli->priority);
              config.writeEntry("style",mli->style);
              config.writeEntry("author",mli->author);
              config.writeEntry("license",mli->license);
              config.writeEntry("indenter",mli->indenter);
              config.writeEntry("hidden",mli->hidden);

              // modified time to keep cache in sync
              config.writeEntry("lastModified", lastModified);

              // Now that the data is in the config file, translate section
              mli->section    = i18nc("Language Section",mli->section.toUtf8().data());
              mli->nameTranslated = i18nc("Language",mli->name.toUtf8().data());

              // Append the new item to the list.
              myModeList.append(mli);
            }
          }
        }
        else
        {
          KateSyntaxModeListItem *emli=new KateSyntaxModeListItem;

          emli->section=i18n("Errors!");
          emli->mimetype= QString::fromLatin1("invalid_file/invalid_file");
          emli->extension=QString::fromLatin1("invalid_file.invalid_file");
          emli->version= QString::fromLatin1("1.");
          emli->name=QString::fromLatin1("Error: %1").arg(*it); // internal
          emli->nameTranslated=i18n("Error: %1", *it); // translated
          emli->identifier=(*it);

          myModeList.append(emli);
        }
      }
    }
  }

  // Synchronize with the file katesyntax...rc
  generalConfig.sync();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
