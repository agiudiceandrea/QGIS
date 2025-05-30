/***************************************************************************
  qgsstackeddiagramproperties.h
  Properties for stacked diagram layers
  -------------------
         begin                : August 2024
         copyright            : (C) Germán Carrillo
         email                : german at opengis dot ch

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "diagram/qgshistogramdiagram.h"
#include "diagram/qgspiediagram.h"
#include "diagram/qgstextdiagram.h"
#include "diagram/qgsstackedbardiagram.h"
#include "diagram/qgsstackeddiagram.h"

#include "qgsgui.h"
#include "qgsdiagramproperties.h"
#include "qgslabelengineconfigdialog.h"
#include "qgsproject.h"
#include "qgsstackeddiagramproperties.h"
#include "moc_qgsstackeddiagramproperties.cpp"
#include "qgsvectorlayer.h"
#include "qgshelp.h"

#include <QMimeData>

QgsStackedDiagramProperties::QgsStackedDiagramProperties( QgsVectorLayer *layer, QWidget *parent, QgsMapCanvas *canvas )
  : QgsPanelWidget( parent )
  , mLayer( layer )
  , mMapCanvas( canvas )
{
  if ( !layer )
  {
    return;
  }

  setupUi( this );
  connect( mSubDiagramsView, &QAbstractItemView::doubleClicked, this, static_cast<void ( QgsStackedDiagramProperties::* )( const QModelIndex & )>( &QgsStackedDiagramProperties::editSubDiagramRenderer ) );

  connect( mAddSubDiagramButton, &QPushButton::clicked, this, &QgsStackedDiagramProperties::addSubDiagramRenderer );
  connect( mEditSubDiagramButton, &QAbstractButton::clicked, this, static_cast<void ( QgsStackedDiagramProperties::* )()>( &QgsStackedDiagramProperties::editSubDiagramRenderer ) );
  connect( mRemoveSubDiagramButton, &QPushButton::clicked, this, &QgsStackedDiagramProperties::removeSubDiagramRenderer );

  // Initialize stacked diagram controls
  mStackedDiagramModeComboBox->addItem( tr( "Horizontal" ), QgsDiagramSettings::Horizontal );
  mStackedDiagramModeComboBox->addItem( tr( "Vertical" ), QgsDiagramSettings::Vertical );

  mStackedDiagramSpacingSpinBox->setClearValue( 0 );
  mStackedDiagramSpacingUnitComboBox->setUnits( { Qgis::RenderUnit::Millimeters, Qgis::RenderUnit::MetersInMapUnits, Qgis::RenderUnit::MapUnits, Qgis::RenderUnit::Pixels, Qgis::RenderUnit::Points, Qgis::RenderUnit::Inches } );

  connect( mStackedDiagramModeComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, &QgsStackedDiagramProperties::widgetChanged );
  connect( mStackedDiagramSpacingSpinBox, qOverload<double>( &QgsDoubleSpinBox::valueChanged ), this, &QgsStackedDiagramProperties::widgetChanged );
  connect( mStackedDiagramSpacingUnitComboBox, &QgsUnitSelectionWidget::changed, this, &QgsStackedDiagramProperties::widgetChanged );

  mModel = new QgsStackedDiagramPropertiesModel();
  mSubDiagramsView->setModel( mModel );

  mSubDiagramsView->setStyle( new QgsStackedDiagramsViewStyle( mSubDiagramsView ) );

  connect( mModel, &QAbstractItemModel::dataChanged, this, &QgsStackedDiagramProperties::widgetChanged );
  connect( mModel, &QAbstractItemModel::rowsInserted, this, &QgsStackedDiagramProperties::widgetChanged );
  connect( mModel, &QAbstractItemModel::rowsRemoved, this, &QgsStackedDiagramProperties::widgetChanged );

  connect( mModel, &QgsStackedDiagramPropertiesModel::subDiagramsMoved, this, &QgsStackedDiagramProperties::clearCurrentIndex );

  syncToLayer();
}

void QgsStackedDiagramProperties::addSubDiagramRenderer()
{
  // Create a single category renderer by default
  std::unique_ptr<QgsDiagramRenderer> renderer;
  auto dr = std::make_unique<QgsSingleCategoryDiagramRenderer>();
  renderer = std::move( dr );

  QItemSelectionModel *sel = mSubDiagramsView->selectionModel();
  const QModelIndex index = sel->currentIndex();

  if ( index.isValid() )
  {
    // add after this subDiagram
    const QModelIndex currentIndex = mSubDiagramsView->selectionModel()->currentIndex();
    mModel->insertSubDiagram( currentIndex.row() + 1, renderer.release() );
    const QModelIndex newIndex = mModel->index( currentIndex.row() + 1, 0 );
    mSubDiagramsView->selectionModel()->setCurrentIndex( newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
  }
  else
  {
    // append to root
    appendSubDiagramRenderer( renderer.release() );
  }
  editSubDiagramRenderer();
}

void QgsStackedDiagramProperties::appendSubDiagramRenderer( QgsDiagramRenderer *dr )
{
  const int rows = mModel->rowCount();
  mModel->insertSubDiagram( rows, dr ); // Transfers ownership
  const QModelIndex newIndex = mModel->index( rows, 0 );
  mSubDiagramsView->selectionModel()->setCurrentIndex( newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
}

void QgsStackedDiagramProperties::editSubDiagramRenderer()
{
  editSubDiagramRenderer( mSubDiagramsView->selectionModel()->currentIndex() );
}

void QgsStackedDiagramProperties::editSubDiagramRenderer( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  QgsDiagramRenderer *renderer = mModel->subDiagramForIndex( index );
  QgsDiagramLayerSettings dls = mModel->diagramLayerSettings();
  QgsPanelWidget *panel = QgsPanelWidget::findParentPanel( this );

  if ( panel && panel->dockMode() )
  {
    QgsDiagramProperties *widget = new QgsDiagramProperties( mLayer, this, mMapCanvas );
    widget->setPanelTitle( tr( "Edit Sub Diagram" ) );
    widget->layout()->setContentsMargins( 0, 0, 0, 0 );
    widget->syncToRenderer( renderer );
    widget->syncToSettings( &dls );
    if ( !couldBeFirstSubDiagram( index ) )
    {
      widget->setAllowedToEditDiagramLayerSettings( false );
    }

    connect( widget, &QgsDiagramProperties::auxiliaryFieldCreated, this, &QgsStackedDiagramProperties::auxiliaryFieldCreated );
    connect( widget, &QgsPanelWidget::panelAccepted, this, &QgsStackedDiagramProperties::subDiagramWidgetPanelAccepted );
    connect( widget, &QgsDiagramProperties::widgetChanged, this, &QgsStackedDiagramProperties::liveUpdateSubDiagramFromPanel );
    openPanel( widget );
    return;
  }

  QgsStackedDiagramPropertiesDialog dlg( mLayer, this, mMapCanvas );
  dlg.syncToRenderer( renderer );
  dlg.syncToSettings( &dls );
  if ( !couldBeFirstSubDiagram( index ) )
  {
    dlg.setAllowedToEditDiagramLayerSettings( false );
  }

  if ( dlg.exec() )
  {
    const QModelIndex index = mSubDiagramsView->selectionModel()->currentIndex();
    if ( dlg.isAllowedToEditDiagramLayerSettings() )
      mModel->updateDiagramLayerSettings( dlg.diagramLayerSettings() );

    // This call will emit dataChanged, which in turns triggers widgetChanged()
    mModel->updateSubDiagram( index, dlg.renderer() );
  }
}

void QgsStackedDiagramProperties::removeSubDiagramRenderer()
{
  const QItemSelection sel = mSubDiagramsView->selectionModel()->selection();
  const auto constSel = sel;
  for ( const QItemSelectionRange &range : constSel )
  {
    if ( range.isValid() )
      mModel->removeRows( range.top(), range.bottom() - range.top() + 1, range.parent() );
  }
  // make sure that the selection is gone
  mSubDiagramsView->selectionModel()->clear();
}

void QgsStackedDiagramProperties::clearCurrentIndex()
{
  mSubDiagramsView->selectionModel()->clearCurrentIndex();
}

void QgsStackedDiagramProperties::syncToLayer()
{
  const QgsDiagramRenderer *dr = mLayer->diagramRenderer();

  if ( dr && dr->diagram() )
  {
    const QList<QgsDiagramSettings> settingList = dr->diagramSettings();
    mStackedDiagramModeComboBox->setCurrentIndex( settingList.at( 0 ).stackedDiagramMode );
    mStackedDiagramSpacingSpinBox->setValue( settingList.at( 0 ).stackedDiagramSpacing() );
    mStackedDiagramSpacingUnitComboBox->setUnit( settingList.at( 0 ).stackedDiagramSpacingUnit() );

    if ( dr->rendererName() == QgsStackedDiagramRenderer::DIAGRAM_RENDERER_NAME_STACKED )
    {
      const QgsStackedDiagramRenderer *stackedDiagramRenderer = static_cast<const QgsStackedDiagramRenderer *>( dr );
      const QList<QgsDiagramRenderer *> renderers = stackedDiagramRenderer->renderers();
      for ( const QgsDiagramRenderer *renderer : renderers )
      {
        appendSubDiagramRenderer( renderer->clone() );
      }
    }
    else
    {
      // Take this single renderer as the first stacked renderer
      appendSubDiagramRenderer( dr->clone() );
    }

    const QgsDiagramLayerSettings *dls = mLayer->diagramLayerSettings();
    mModel->updateDiagramLayerSettings( *dls );

    mSubDiagramsView->selectionModel()->clear();
  }
}

void QgsStackedDiagramProperties::apply()
{
  auto ds = std::make_unique<QgsDiagramSettings>();
  ds->stackedDiagramMode = static_cast<QgsDiagramSettings::StackedDiagramMode>( mStackedDiagramModeComboBox->currentData().toInt() );
  ds->setStackedDiagramSpacingUnit( mStackedDiagramSpacingUnitComboBox->unit() );
  ds->setStackedDiagramSpacing( mStackedDiagramSpacingSpinBox->value() );

  // Create diagram renderer for the StackedDiagram
  QgsStackedDiagramRenderer *dr = new QgsStackedDiagramRenderer();
  dr->setDiagram( new QgsStackedDiagram() );

  // Get DiagramSettings from each subdiagram
  const QList<QgsDiagramRenderer *> renderers = mModel->subRenderers();
  for ( const QgsDiagramRenderer *renderer : renderers )
  {
    const QList<QgsDiagramSettings> ds1 = renderer->diagramSettings();
    if ( !ds1.isEmpty() )
    {
      ds->categoryAttributes += ds1.at( 0 ).categoryAttributes;
      ds->categoryLabels += ds1.at( 0 ).categoryLabels;
      ds->categoryColors += ds1.at( 0 ).categoryColors;
    }
    dr->addRenderer( renderer->clone() );
  }

  dr->setDiagramSettings( *ds );
  mLayer->setDiagramRenderer( dr );

  // Get DiagramLayerSettings from the model
  QgsDiagramLayerSettings dls = mModel->diagramLayerSettings();
  mLayer->setDiagramLayerSettings( dls );

  // refresh
  QgsProject::instance()->setDirty( true );
  mLayer->triggerRepaint();
}

bool QgsStackedDiagramProperties::couldBeFirstSubDiagram( const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return false;

  if ( mModel->rowCount() == 1 )
    return true;

  // Is there any enabled subdiagram before our index.row()?
  // If so, ours cannot be the first diagram.
  const QList<QgsDiagramRenderer *> renderers = mModel->subRenderers();

  for ( int i = 0; i < index.row(); i++ )
  {
    const QgsDiagramRenderer *renderer = renderers.at( i );
    const QList<QgsDiagramSettings> ds = renderer->diagramSettings();
    if ( !ds.isEmpty() && ds.at( 0 ).enabled )
    {
      // First enabled subdiagram found, and we know our row is after.
      // Therefore, return false to disallow showing DLS settings for it.
      return false;
    }
  }
  // Either our row is the first subdiagram enabled or it's disabled,
  // but there are no enabled ones before. So, ours could be the first
  // enabled one after being edited.
  // Therefore, we should allow DLS settings on its corresponding widget.
  return true;
}

void QgsStackedDiagramProperties::subDiagramWidgetPanelAccepted( QgsPanelWidget *panel )
{
  QgsDiagramProperties *widget = qobject_cast<QgsDiagramProperties *>( panel );

  std::unique_ptr<QgsDiagramRenderer> renderer = widget->createRenderer();

  const QModelIndex index = mSubDiagramsView->selectionModel()->currentIndex();
  if ( widget->isAllowedToEditDiagramLayerSettings() )
    mModel->updateDiagramLayerSettings( widget->createDiagramLayerSettings() );

  mModel->updateSubDiagram( index, renderer.release() );
}

void QgsStackedDiagramProperties::liveUpdateSubDiagramFromPanel()
{
  subDiagramWidgetPanelAccepted( qobject_cast<QgsPanelWidget *>( sender() ) );
}

////

#include "qgsvscrollarea.h"

QgsStackedDiagramPropertiesDialog::QgsStackedDiagramPropertiesDialog( QgsVectorLayer *layer, QWidget *parent, QgsMapCanvas *mapCanvas )
  : QDialog( parent )
{
#ifdef Q_OS_MAC
  setWindowModality( Qt::WindowModal );
#endif

  QVBoxLayout *layout = new QVBoxLayout( this );
  QgsVScrollArea *scrollArea = new QgsVScrollArea( this );
  scrollArea->setFrameShape( QFrame::NoFrame );
  layout->addWidget( scrollArea );

  buttonBox = new QDialogButtonBox( QDialogButtonBox::Cancel | QDialogButtonBox::Help | QDialogButtonBox::Ok );
  mPropsWidget = new QgsDiagramProperties( layer, this, mapCanvas );
  mPropsWidget->setDockMode( false );

  scrollArea->setWidget( mPropsWidget );
  layout->addWidget( buttonBox );
  this->setWindowTitle( "Edit Sub Diagram" );
  QgsGui::enableAutoGeometryRestore( this );

  connect( buttonBox, &QDialogButtonBox::accepted, this, &QgsStackedDiagramPropertiesDialog::accept );
  connect( buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  connect( buttonBox, &QDialogButtonBox::helpRequested, this, &QgsStackedDiagramPropertiesDialog::showHelp );
}

void QgsStackedDiagramPropertiesDialog::syncToRenderer( const QgsDiagramRenderer *dr ) const
{
  mPropsWidget->syncToRenderer( dr );
}

void QgsStackedDiagramPropertiesDialog::syncToSettings( const QgsDiagramLayerSettings *dls ) const
{
  mPropsWidget->syncToSettings( dls );
}

void QgsStackedDiagramPropertiesDialog::accept()
{
  // Get renderer and diagram layer settings from widget
  mRenderer = mPropsWidget->createRenderer();
  mDiagramLayerSettings = mPropsWidget->createDiagramLayerSettings();
  QDialog::accept();
}

QgsDiagramRenderer *QgsStackedDiagramPropertiesDialog::renderer()
{
  return mRenderer.release();
}

QgsDiagramLayerSettings QgsStackedDiagramPropertiesDialog::diagramLayerSettings() const
{
  return mDiagramLayerSettings;
}

void QgsStackedDiagramPropertiesDialog::setAllowedToEditDiagramLayerSettings( bool allowed ) const
{
  mPropsWidget->setAllowedToEditDiagramLayerSettings( allowed );
}

bool QgsStackedDiagramPropertiesDialog::isAllowedToEditDiagramLayerSettings() const
{
  return mPropsWidget->isAllowedToEditDiagramLayerSettings();
}

void QgsStackedDiagramPropertiesDialog::showHelp()
{
  QgsHelp::openHelp( QStringLiteral( "working_with_vector/vector_properties.html#diagrams-properties" ) );
}

////

QgsStackedDiagramPropertiesModel::QgsStackedDiagramPropertiesModel( QObject *parent )
  : QAbstractTableModel( parent )
{
}

QgsStackedDiagramPropertiesModel::~QgsStackedDiagramPropertiesModel()
{
  qDeleteAll( mRenderers );
}

Qt::ItemFlags QgsStackedDiagramPropertiesModel::flags( const QModelIndex &index ) const
{
  // Flat list, to ease drop handling valid indexes are not dropEnabled
  if ( !index.isValid() )
    return Qt::ItemIsDropEnabled;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

  if ( index.column() == 0 )
    flags |= Qt::ItemIsUserCheckable;

  return flags;
}

Qt::DropActions QgsStackedDiagramPropertiesModel::supportedDropActions() const
{
  return Qt::MoveAction; // | Qt::CopyAction
}

QStringList QgsStackedDiagramPropertiesModel::mimeTypes() const
{
  QStringList types;
  types << QStringLiteral( "application/vnd.text.list" );
  return types;
}

QMimeData *QgsStackedDiagramPropertiesModel::mimeData( const QModelIndexList &indexes ) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream( &encodedData, QIODevice::WriteOnly );

  // Sort indexes since their order reflects selection order
  QModelIndexList sortedIndexes = indexes;
  std::sort( sortedIndexes.begin(), sortedIndexes.end() );

  for ( const QModelIndex &index : std::as_const( sortedIndexes ) )
  {
    // each item consists of several columns - let's add it with just first one
    if ( !index.isValid() || index.column() != 0 )
      continue;

    if ( QgsDiagramRenderer *diagram = mRenderers.at( index.row() ) )
    {
      QDomDocument doc;

      QDomElement rootElem = doc.createElement( QStringLiteral( "diagram_mime" ) );
      diagram->writeXml( rootElem, doc, QgsReadWriteContext() );
      doc.appendChild( rootElem );
      stream << doc.toString( -1 );
    }
  }

  mimeData->setData( QStringLiteral( "application/vnd.text.list" ), encodedData );
  return mimeData;
}

bool QgsStackedDiagramPropertiesModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent )
{
  Q_UNUSED( column )
  Q_UNUSED( parent )

  if ( action == Qt::IgnoreAction )
    return true;

  if ( !data->hasFormat( QStringLiteral( "application/vnd.text.list" ) ) )
    return false;

  QByteArray encodedData = data->data( QStringLiteral( "application/vnd.text.list" ) );
  QDataStream stream( &encodedData, QIODevice::ReadOnly );
  int rows = 0;

  // the item was dropped at parent, we may decide where to put the items
  if ( row == -1 )
  {
    row = mRenderers.count(); // let's append them (e.g., drop in empty space)
  }

  while ( !stream.atEnd() )
  {
    QString text;
    stream >> text;

    QDomDocument doc;
    if ( !doc.setContent( text ) )
      continue;
    const QDomElement rootElem = doc.documentElement();
    if ( rootElem.tagName() != QLatin1String( "diagram_mime" ) || !rootElem.hasChildNodes() )
      continue;
    const QDomElement childElem = rootElem.firstChild().toElement();

    QgsDiagramRenderer *diagram = nullptr;
    if ( childElem.nodeName() == QLatin1String( "SingleCategoryDiagramRenderer" ) )
    {
      diagram = new QgsSingleCategoryDiagramRenderer();
      diagram->readXml( childElem, QgsReadWriteContext() );
    }
    else if ( childElem.nodeName() == QLatin1String( "LinearlyInterpolatedDiagramRenderer" ) )
    {
      diagram = new QgsLinearlyInterpolatedDiagramRenderer();
      diagram->readXml( childElem, QgsReadWriteContext() );
    }
    else if ( childElem.nodeName() == QLatin1String( "StackedDiagramRenderer" ) )
    {
      diagram = new QgsStackedDiagramRenderer();
      diagram->readXml( childElem, QgsReadWriteContext() );
    }

    if ( diagram )
    {
      insertSubDiagram( row + rows, diagram );
      rows++;
    }
  }

  emit subDiagramsMoved(); // Let views know they can clean some things up
  return true;
}

QVariant QgsStackedDiagramPropertiesModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  QgsDiagramRenderer *dr = subDiagramForIndex( index );

  if ( role == Qt::DisplayRole || role == Qt::ToolTipRole )
  {
    switch ( index.column() )
    {
      case 0:
        if ( dr && dr->diagram() )
        {
          if ( dr->diagram()->diagramName() == QgsPieDiagram::DIAGRAM_NAME_PIE )
          {
            return tr( "Pie Chart" );
          }
          else if ( dr->diagram()->diagramName() == QgsTextDiagram::DIAGRAM_NAME_TEXT )
          {
            return tr( "Text Diagram" );
          }
          else if ( dr->diagram()->diagramName() == QgsHistogramDiagram::DIAGRAM_NAME_HISTOGRAM )
          {
            return tr( "Histogram" );
          }
          else if ( dr->diagram()->diagramName() == QgsStackedBarDiagram::DIAGRAM_NAME_STACKED_BAR )
          {
            return tr( "Stacked Bars" );
          }
          else if ( dr->diagram()->diagramName() == QgsStackedDiagram::DIAGRAM_NAME_STACKED )
          {
            return tr( "Stacked Diagram" );
          }
          else
          {
            return dr->diagram()->diagramName();
          }
        }
        else
        {
          return tr( "(no diagram)" );
        }
      case 1:
        if ( !dr )
        {
          return tr( "(no renderer)" );
        }
        else
        {
          if ( dr->rendererName() == QgsSingleCategoryDiagramRenderer::DIAGRAM_RENDERER_NAME_SINGLE_CATEGORY )
            return tr( "Fixed" );
          else if ( dr->rendererName() == QgsLinearlyInterpolatedDiagramRenderer::DIAGRAM_RENDERER_NAME_LINEARLY_INTERPOLATED )
            return tr( "Scaled" );
          else
            return tr( "Unknown" );
        }
      case 2:
        if ( dr && dr->diagram() && !dr->diagramSettings().isEmpty() )
        {
          if ( QgsHistogramDiagram::DIAGRAM_NAME_HISTOGRAM == dr->diagram()->diagramName() || QgsStackedBarDiagram::DIAGRAM_NAME_STACKED_BAR == dr->diagram()->diagramName() )
          {
            switch ( dr->diagramSettings().at( 0 ).diagramOrientation )
            {
              case QgsDiagramSettings::Left:
                return tr( "Left" );
              case QgsDiagramSettings::Right:
                return tr( "Right" );
              case QgsDiagramSettings::Up:
                return tr( "Up" );
              case QgsDiagramSettings::Down:
                return tr( "Down" );
            }
          }
        }
        return QVariant();
      default:
        return QVariant();
    }
  }
  else if ( role == Qt::CheckStateRole )
  {
    if ( index.column() != 0 )
      return QVariant();

    return ( dr && !dr->diagramSettings().isEmpty() && dr->diagramSettings().at( 0 ).enabled ) ? Qt::Checked : Qt::Unchecked;
  }
  else
  {
    return QVariant();
  }
}

QVariant QgsStackedDiagramPropertiesModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 && section < 3 )
  {
    QStringList lst;
    lst << tr( "Diagram type" ) << tr( "Size" ) << tr( "Orientation" );
    return lst[section];
  }

  return QVariant();
}

int QgsStackedDiagramPropertiesModel::rowCount( const QModelIndex & ) const
{
  return mRenderers.size();
}

int QgsStackedDiagramPropertiesModel::columnCount( const QModelIndex & ) const
{
  return 3;
}

bool QgsStackedDiagramPropertiesModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( !index.isValid() )
    return false;

  QgsDiagramRenderer *dr = subDiagramForIndex( index );

  if ( role == Qt::CheckStateRole )
  {
    if ( dr && !dr->diagramSettings().isEmpty() )
    {
      QgsDiagramSettings ds = dr->diagramSettings().at( 0 );
      ds.enabled = ( value.toInt() == Qt::Checked );

      if ( dr->rendererName() == QgsSingleCategoryDiagramRenderer::DIAGRAM_RENDERER_NAME_SINGLE_CATEGORY )
      {
        QgsSingleCategoryDiagramRenderer *dsr = static_cast<QgsSingleCategoryDiagramRenderer *>( dr );
        dsr->setDiagramSettings( ds );
      }
      else
      {
        QgsLinearlyInterpolatedDiagramRenderer *dlir = static_cast<QgsLinearlyInterpolatedDiagramRenderer *>( dr );
        dlir->setDiagramSettings( ds );
      }

      emit dataChanged( index, index );
      return true;
    }
  }
  return false;
}

bool QgsStackedDiagramPropertiesModel::removeRows( int row, int count, const QModelIndex &parent )
{
  if ( row < 0 || row >= mRenderers.size() )
    return false;

  beginRemoveRows( parent, row, row + count - 1 );
  while ( count-- )
    mRenderers.removeAt( row );
  endRemoveRows();

  return true;
}

QgsDiagramRenderer *QgsStackedDiagramPropertiesModel::subDiagramForIndex( const QModelIndex &index ) const
{
  if ( index.isValid() )
    return mRenderers.at( index.row() );
  return nullptr;
}

void QgsStackedDiagramPropertiesModel::insertSubDiagram( const int index, QgsDiagramRenderer *newSubDiagram )
{
  beginInsertRows( QModelIndex(), index, index );
  mRenderers.insert( index, newSubDiagram );
  endInsertRows();
}

void QgsStackedDiagramPropertiesModel::updateSubDiagram( const QModelIndex &index, QgsDiagramRenderer *dr )
{
  if ( !index.isValid() )
    return;

  delete mRenderers.at( index.row() );
  mRenderers.replace( index.row(), dr );
  emit dataChanged( index, index );
}

QList<QgsDiagramRenderer *> QgsStackedDiagramPropertiesModel::subRenderers() const
{
  return mRenderers;
}

void QgsStackedDiagramPropertiesModel::updateDiagramLayerSettings( QgsDiagramLayerSettings dls )
{
  mDiagramLayerSettings = dls;
}

QgsDiagramLayerSettings QgsStackedDiagramPropertiesModel::diagramLayerSettings() const
{
  return mDiagramLayerSettings;
}

// ------------------------------ View style --------------------------------
QgsStackedDiagramsViewStyle::QgsStackedDiagramsViewStyle( QWidget *parent )
  : QgsProxyStyle( parent )
{}

void QgsStackedDiagramsViewStyle::drawPrimitive( PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget ) const
{
  if ( element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull() )
  {
    QStyleOption opt( *option );
    opt.rect.setLeft( 0 );
    // draw always as line above, because we move item to that index
    opt.rect.setHeight( 0 );
    if ( widget )
      opt.rect.setRight( widget->width() );
    QProxyStyle::drawPrimitive( element, &opt, painter, widget );
    return;
  }
  QProxyStyle::drawPrimitive( element, option, painter, widget );
}
