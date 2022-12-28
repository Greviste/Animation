#include "AnimationListModel.h"


void AnimationListModel::addAnimation(std::unique_ptr<SimpleAnimation> anim)
{
    if(!anim) return;
    beginInsertRows(QModelIndex(), _animations.size(), _animations.size());
    _animations.emplace_back(std::move(anim));
    endInsertRows();
}

void AnimationListModel::clear()
{
    beginResetModel();
    _animations.clear();
    endResetModel();
}

SimpleAnimation& AnimationListModel::operator[](int i) const
{
    return *_animations[i];
}

int AnimationListModel::rowCount(const QModelIndex& parent) const
{
    return _animations.size();
}

QVariant AnimationListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || role != Qt::DisplayRole)
        return {};
    
    return QString(_animations[index.row()]->name().c_str());
}

Qt::ItemFlags AnimationListModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags result = QAbstractListModel::flags(index);
    if(index.isValid())
        result |= Qt::ItemFlag::ItemIsDragEnabled;
    
    return result;
}
