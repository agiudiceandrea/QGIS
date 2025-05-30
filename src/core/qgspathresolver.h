/***************************************************************************
  qgspathresolver.h
  --------------------------------------
  Date                 : February 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPATHRESOLVER_H
#define QGSPATHRESOLVER_H

#include "qgis_core.h"

#include <QString>
#include <functional>
#include <vector>

/**
 * \ingroup core
 * \brief Resolves relative paths into absolute paths and vice versa.
 *
 * Used for writing and reading paths for projects.
 */
class CORE_EXPORT QgsPathResolver
{
  public:
    //! Initialize path resolver with a base filename. Null filename means no conversion between relative/absolute path
    explicit QgsPathResolver( const QString &baseFileName = QString(), const QString &attachmentDir = QString() );

    /**
     * Prepare a filename to save it to the project file.
     * Creates an absolute or relative path according to the project settings.
     * Paths written to the project file should be prepared with this method.
    */
    QString writePath( const QString &filename ) const;

    //! Turn filename read from the project file to an absolute path
    QString readPath( const QString &filename ) const;

#ifndef SIP_RUN

    /**
     * Sets a path pre-processor function, which allows for manipulation of paths and data sources prior
     * to resolving them to file references or layer sources.
     *
     * The \a processor function must accept a single string argument (representing the original file path
     * or data source), and return a processed version of this path.
     *
     * The path pre-processor function is called before any bad layer handler.
     *
     * If multiple preprocessors are set, they will be called in sequence based on the order in which
     * they were originally set.
     *
     * \returns An auto-generated string uniquely identifying the preprocessor, which can later be
     * used to remove the processor (via a call to removePathPreprocessor()).
     *
     * \see removePathPreprocessor()
     * \since QGIS 3.10
     */
    static QString setPathPreprocessor( const std::function< QString( const QString &filename )> &processor );
#else

    /**
     * Sets a path pre-processor function, which allows for manipulation of paths and data sources prior
     * to resolving them to file references or layer sources.
     *
     * The \a processor function must accept a single string argument (representing the original file path
     * or data source), and return a processed version of this path.
     *
     * The path pre-processor function is called before any bad layer handler.
     *
     * If multiple preprocessors are set, they will be called in sequence based on the order in which
     * they were originally set.
     *
     * Example - replace an outdated folder path with a new one:
     *
     * \code{.py}
     *   def my_processor(path):
     *      return path.replace('c:/Users/ClintBarton/Documents/Projects', 'x:/Projects/')
     *
     *   QgsPathResolver.setPathPreprocessor(my_processor)
     * \endcode
     *
     * Example - replace a stored database host with a new one:
     *
     * \code{.py}
     *   def my_processor(path):
     *      return path.replace('host=10.1.1.115', 'host=10.1.1.116')
     *
     *   QgsPathResolver.setPathPreprocessor(my_processor)
     * \endcode
     *
     * Example - replace stored database credentials with new ones:
     *
     * \code{.py}
     *   def my_processor(path):
     *      path = path.replace("user='gis_team'", "user='team_awesome'")
     *      path = path.replace("password='cats'", "password='g7as!m*'")
     *      return path
     *
     *   QgsPathResolver.setPathPreprocessor(my_processor)
     * \endcode
     *
     * \since QGIS 3.10
     */
    static QString setPathPreprocessor( SIP_PYCALLABLE / AllowNone / );
    % MethodCode
    PyObject *s = 0;
    QString id;
    Py_XINCREF( a0 );
    Py_BEGIN_ALLOW_THREADS
    id = QgsPathResolver::setPathPreprocessor( [a0]( const QString &arg )->QString
    {
      QString res;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a0, "D", &arg, sipType_QString, NULL );
      int state;
      int sipIsError = 0;
      QString *t1 = reinterpret_cast<QString *>( sipConvertToType( s, sipType_QString, 0, SIP_NOT_NONE, &state, &sipIsError ) );
      if ( sipIsError == 0 )
      {
        res = QString( *t1 );
      }
      sipReleaseType( t1, sipType_QString, state );
      SIP_UNBLOCK_THREADS
      return res;
    } );

    Py_END_ALLOW_THREADS
    s = sipConvertFromNewType( new QString( id ), sipType_QString, 0 );
    return s;
    % End
#endif

#ifndef SIP_RUN

    /**
     * Removes the custom pre-processor function with matching \a id.
     *
     * The \a id must correspond to a pre-processor previously added via a call to setPathPreprocessor().
     *
     * Returns TRUE if processor existed and was removed.
     *
     * \see setPathPreprocessor()
     * \since QGIS 3.10
     */
    static bool removePathPreprocessor( const QString &id );
#else

    /**
     * Removes the custom pre-processor function with matching \a id.
     *
     * The \a id must correspond to a pre-processor previously added via a call to setPathPreprocessor().
     *
     * \throws KeyError if no processor with the specified \a id exists.
     *
     * \see setPathPreprocessor()
     * \since QGIS 3.10
     */
    static void removePathPreprocessor( const QString &id );
    % MethodCode
    if ( !QgsPathResolver::removePathPreprocessor( *a0 ) )
    {
      PyErr_SetString( PyExc_KeyError, QStringLiteral( "No processor with id %1 exists." ).arg( *a0 ).toUtf8().constData() );
      sipIsErr = 1;
    }
    % End
#endif



    /**
     * Sets a path writer function, which allows for manipulation of paths and data sources prior
     * to writing them to the project file.
     *
     * The \a writer function must accept a single string argument (representing the original file path
     * or data source), and return a processed version of this path.
     *
     * The path writer function is called before any bad layer handler.
     *
     * If multiple writers are set, they will be called in sequence based on the order in which
     * they were originally set.
     *
     * \returns An auto-generated string uniquely identifying the writer, which can later be
     * used to remove the writer (via a call to removePathWriter()).
     *
     * \see removePathWriter()
     * \since QGIS 3.22
     */
#ifndef SIP_RUN
    static QString setPathWriter( const std::function< QString( const QString &filename )> &writer );
#else

    /**
     * Sets a path writer function, which allows for manipulation of paths and data sources prior
     * to writing them to the project file.
     *
     * The \a writer function must accept a single string argument (representing the original file path
     * or data source), and return a processed version of this path.
     *
     * The path writer function is called before any bad layer handler.
     *
     * If multiple writers are set, they will be called in sequence based on the order in which
     * they were originally set.
     *
     * Example - replace path with a variable:
     *
     * \code{.py}
     *   def my_processor(path):
     *      return path.replace('c:/Users/ClintBarton/Documents/Projects', '$projectdir$')
     *
     *   QgsPathResolver.setPathWriter(my_processor)
     * \endcode
     *
     * \since QGIS 3.22
     */
    static QString setPathWriter( SIP_PYCALLABLE / AllowNone / );
    % MethodCode
    PyObject *s = 0;
    QString id;
    Py_XINCREF( a0 );
    Py_BEGIN_ALLOW_THREADS
    id = QgsPathResolver::setPathWriter( [a0]( const QString &arg )->QString
    {
      QString res;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a0, "D", &arg, sipType_QString, NULL );
      int state;
      int sipIsError = 0;
      QString *t1 = reinterpret_cast<QString *>( sipConvertToType( s, sipType_QString, 0, SIP_NOT_NONE, &state, &sipIsError ) );
      if ( sipIsError == 0 )
      {
        res = QString( *t1 );
      }
      sipReleaseType( t1, sipType_QString, state );
      SIP_UNBLOCK_THREADS
      return res;
    } );

    Py_END_ALLOW_THREADS
    s = sipConvertFromNewType( new QString( id ), sipType_QString, 0 );
    return s;
    % End
#endif

    /**
     * Removes the custom writer function with matching \a id.
     *
     * The \a id must correspond to a writer previously added via a call to setPathWriter().
     *
     * Returns TRUE if writer existed and was removed.
     *
     * \see setPathWriter()
     * \since QGIS 3.22
     */
#ifndef SIP_RUN
    static bool removePathWriter( const QString &id );
#else

    /**
     * Removes the custom writer function with matching \a id.
     *
     * The \a id must correspond to a writer previously added via a call to setPathWriter().
     * An KeyError will be raised if no processor with the specified \a id exists.
     *
     * \see setPathWriter()
     * \since QGIS 3.22
     */
    static void removePathWriter( const QString &id );
    % MethodCode
    if ( !QgsPathResolver::removePathWriter( *a0 ) )
    {
      PyErr_SetString( PyExc_KeyError, QStringLiteral( "No writer with id %1 exists." ).arg( *a0 ).toUtf8().constData() );
      sipIsErr = 1;
    }
    % End
#endif

  private:
    //! path to a file that is the base for relative path resolution
    QString mBaseFileName;
    //! path where attached files are stored
    QString mAttachmentDir;
};

#endif // QGSPATHRESOLVER_H
