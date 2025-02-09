#include <QGuiApplication>
#include <QMenu>
#include <QXmlStreamWriter>
#include <QClipboard>
#include <QApplication>
#include <dust3d/base/snapshot_xml.h>
#include "material_list_widget.h"
#include "document.h"

MaterialListWidget::MaterialListWidget(const Document *document, QWidget *parent) :
    QTreeWidget(parent),
    m_document(document)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAutoScroll(false);

    setHeaderHidden(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);

    setStyleSheet("QTreeView {qproperty-indentation: 0;}");

    setContentsMargins(0, 0, 0, 0);

    connect(document, &Document::materialListChanged, this, &MaterialListWidget::reload);
    connect(document, &Document::cleanup, this, &MaterialListWidget::removeAllContent);

    connect(this, &MaterialListWidget::removeMaterial, document, &Document::removeMaterial);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &MaterialListWidget::showContextMenu);

    reload();
}

void MaterialListWidget::enableMultipleSelection(bool enabled)
{
    m_multipleSelectionEnabled = enabled;
}

void MaterialListWidget::materialRemoved(dust3d::Uuid materialId)
{
    if (m_currentSelectedMaterialId == materialId)
        m_currentSelectedMaterialId = dust3d::Uuid();
    m_selectedMaterialIds.erase(materialId);
    m_itemMap.erase(materialId);
}

void MaterialListWidget::updateMaterialSelectState(dust3d::Uuid materialId, bool selected)
{
    auto findItemResult = m_itemMap.find(materialId);
    if (findItemResult == m_itemMap.end()) {
        qDebug() << "Find material item failed:" << materialId;
        return;
    }
    MaterialWidget *materialWidget = (MaterialWidget *)itemWidget(findItemResult->second.first, findItemResult->second.second);
    materialWidget->updateCheckedState(selected);
}

void MaterialListWidget::selectMaterial(dust3d::Uuid materialId, bool multiple)
{
    if (multiple) {
        if (!m_currentSelectedMaterialId.isNull()) {
            m_selectedMaterialIds.insert(m_currentSelectedMaterialId);
            m_currentSelectedMaterialId = dust3d::Uuid();
        }
        if (m_selectedMaterialIds.find(materialId) != m_selectedMaterialIds.end()) {
            updateMaterialSelectState(materialId, false);
            m_selectedMaterialIds.erase(materialId);
        } else {
            if (m_multipleSelectionEnabled || m_selectedMaterialIds.empty()) {
                updateMaterialSelectState(materialId, true);
                m_selectedMaterialIds.insert(materialId);
            }
        }
        if (m_selectedMaterialIds.size() > 1) {
            return;
        }
        if (m_selectedMaterialIds.size() == 1)
            materialId = *m_selectedMaterialIds.begin();
        else {
            materialId = dust3d::Uuid();
            emit currentSelectedMaterialChanged(materialId);
        }
    }
    if (!m_selectedMaterialIds.empty()) {
        for (const auto &id: m_selectedMaterialIds) {
            updateMaterialSelectState(id, false);
        }
        m_selectedMaterialIds.clear();
    }
    if (m_currentSelectedMaterialId != materialId) {
        if (!m_currentSelectedMaterialId.isNull()) {
            updateMaterialSelectState(m_currentSelectedMaterialId, false);
        }
        m_currentSelectedMaterialId = materialId;
        if (!m_currentSelectedMaterialId.isNull()) {
            updateMaterialSelectState(m_currentSelectedMaterialId, true);
        }
        emit currentSelectedMaterialChanged(m_currentSelectedMaterialId);
    }
}

void MaterialListWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        bool multiple = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);
        if (itemIndex.isValid()) {
            QTreeWidgetItem *item = itemFromIndex(itemIndex);
            auto materialId = dust3d::Uuid(item->data(itemIndex.column(), Qt::UserRole).toString().toUtf8().constData());
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                bool startAdd = false;
                bool stopAdd = false;
                std::vector<dust3d::Uuid> waitQueue;
                for (const auto &childId: m_document->materialIdList) {
                    if (m_shiftStartMaterialId == childId || materialId == childId) {
                        if (startAdd) {
                            stopAdd = true;
                        } else {
                            startAdd = true;
                        }
                    }
                    if (startAdd)
                        waitQueue.push_back(childId);
                    if (stopAdd)
                        break;
                }
                if (stopAdd && !waitQueue.empty()) {
                    if (!m_selectedMaterialIds.empty()) {
                        for (const auto &id: m_selectedMaterialIds) {
                            updateMaterialSelectState(id, false);
                        }
                        m_selectedMaterialIds.clear();
                    }
                    if (!m_currentSelectedMaterialId.isNull()) {
                        m_currentSelectedMaterialId = dust3d::Uuid();
                    }
                    for (const auto &waitId: waitQueue) {
                        selectMaterial(waitId, true);
                    }
                }
                return;
            } else {
                m_shiftStartMaterialId = materialId;
            }
            selectMaterial(materialId, multiple);
            return;
        }
        if (!multiple)
            selectMaterial(dust3d::Uuid());
    }
}

bool MaterialListWidget::isMaterialSelected(dust3d::Uuid materialId)
{
    return (m_currentSelectedMaterialId == materialId ||
        m_selectedMaterialIds.find(materialId) != m_selectedMaterialIds.end());
}

void MaterialListWidget::showContextMenu(const QPoint &pos)
{
    if (!m_hasContextMenu)
        return;

    QMenu contextMenu(this);

    std::set<dust3d::Uuid> unorderedMaterialIds = m_selectedMaterialIds;
    if (!m_currentSelectedMaterialId.isNull())
        unorderedMaterialIds.insert(m_currentSelectedMaterialId);

    std::vector<dust3d::Uuid> materialIds;
    for (const auto &cand: m_document->materialIdList) {
        if (unorderedMaterialIds.find(cand) != unorderedMaterialIds.end())
            materialIds.push_back(cand);
    }

    QAction modifyAction(tr("Modify"), this);
    if (materialIds.size() == 1) {
        connect(&modifyAction, &QAction::triggered, this, [=]() {
            emit modifyMaterial(*materialIds.begin());
        });
        contextMenu.addAction(&modifyAction);
    }

    QAction copyAction(tr("Copy"), this);
    if (!materialIds.empty()) {
        connect(&copyAction, &QAction::triggered, this, &MaterialListWidget::copy);
        contextMenu.addAction(&copyAction);
    }

    QAction pasteAction(tr("Paste"), this);
    if (m_document->hasPastableMaterialsInClipboard()) {
        connect(&pasteAction, &QAction::triggered, m_document, &Document::paste);
        contextMenu.addAction(&pasteAction);
    }

    QAction deleteAction(tr("Delete"), this);
    if (!materialIds.empty()) {
        connect(&deleteAction, &QAction::triggered, [=]() {
            for (const auto &materialId: materialIds)
                emit removeMaterial(materialId);
        });
        contextMenu.addAction(&deleteAction);
    }

    contextMenu.exec(mapToGlobal(pos));
}

void MaterialListWidget::resizeEvent(QResizeEvent *event)
{
    QTreeWidget::resizeEvent(event);
    if (calculateColumnCount() != columnCount())
        reload();
}

int MaterialListWidget::calculateColumnCount()
{
    if (nullptr == parentWidget())
        return 0;

    int columns = parentWidget()->width() / Theme::materialPreviewImageSize;
    if (0 == columns)
        columns = 1;
    return columns;
}

void MaterialListWidget::reload()
{
    removeAllContent();

    int columns = calculateColumnCount();
    if (0 == columns)
        return;

    int columnWidth = parentWidget()->width() / columns;

    //qDebug() << "parentWidth:" << parentWidget()->width() << "columnWidth:" << columnWidth << "columns:" << columns;

    setColumnCount(columns);
    for (int i = 0; i < columns; i++)
        setColumnWidth(i, columnWidth);
    
    std::vector<dust3d::Uuid> orderedMaterialIdList = m_document->materialIdList;
    std::sort(orderedMaterialIdList.begin(), orderedMaterialIdList.end(), [&](const dust3d::Uuid &firstMaterialId, const dust3d::Uuid &secondMaterialId) {
        const auto *firstMaterial = m_document->findMaterial(firstMaterialId);
        const auto *secondMaterial = m_document->findMaterial(secondMaterialId);
        if (nullptr == firstMaterial || nullptr == secondMaterial)
            return false;
        return QString::compare(firstMaterial->name, secondMaterial->name, Qt::CaseInsensitive) < 0;
    });

    decltype(orderedMaterialIdList.size()) materialIndex = 0;
    while (materialIndex < orderedMaterialIdList.size()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(this);
        item->setFlags((item->flags() | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable) & ~(Qt::ItemIsEditable));
        for (int col = 0; col < columns && materialIndex < orderedMaterialIdList.size(); col++, materialIndex++) {
            const auto &materialId = orderedMaterialIdList[materialIndex];
            item->setSizeHint(col, QSize(columnWidth, MaterialWidget::preferredHeight() + 2));
            item->setData(col, Qt::UserRole, QString(materialId.toString().c_str()));
            MaterialWidget *widget = new MaterialWidget(m_document, materialId);
            connect(widget, &MaterialWidget::modifyMaterial, this, &MaterialListWidget::modifyMaterial);
            setItemWidget(item, col, widget);
            widget->reload();
            widget->updateCheckedState(isMaterialSelected(materialId));
            m_itemMap[materialId] = std::make_pair(item, col);
        }
        invisibleRootItem()->addChild(item);
    }
}

void MaterialListWidget::setHasContextMenu(bool hasContextMenu)
{
    m_hasContextMenu = hasContextMenu;
}

void MaterialListWidget::removeAllContent()
{
    m_itemMap.clear();
    clear();
}

void MaterialListWidget::copy()
{
    if (m_selectedMaterialIds.empty() && m_currentSelectedMaterialId.isNull())
        return;

    std::set<dust3d::Uuid> limitMaterialIds = m_selectedMaterialIds;
    if (!m_currentSelectedMaterialId.isNull())
        limitMaterialIds.insert(m_currentSelectedMaterialId);

    std::set<dust3d::Uuid> emptySet;

    dust3d::Snapshot snapshot;
    m_document->toSnapshot(&snapshot, emptySet, DocumentToSnapshotFor::Materials,
        limitMaterialIds);
        
    std::string snapshotXml;
    dust3d::saveSnapshotToXmlString(snapshot, snapshotXml);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml.c_str());
}
