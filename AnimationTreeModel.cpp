#include "AnimationTreeModel.h"


AnimationTreeModel::AnimationTreeModel(std::shared_ptr<const Skeleton> skeleton)
    :_root(std::make_unique<NullAnimation>(std::move(skeleton)))
{
}

Animation& AnimationTreeModel::getAnimation(const QModelIndex& index) const
{
    if(!index.isValid())
        return *_root;
    
    return *static_cast<Animation*>(index.internalPointer());
}

void AnimationTreeModel::addChild(const QModelIndex& index)
{
    if(!index.isValid())
        return;
    
    Animation* parent = static_cast<Animation*>(index.internalPointer());
    CompositeAnimation* as_comp = dynamic_cast<CompositeAnimation*>(parent);
    if(!as_comp)
        return;

    beginInsertRows(index, as_comp->count(), as_comp->count());
    auto i = as_comp->addAnimation(nullptr);
    _parents[&(*as_comp)[i]] = as_comp;
    endInsertRows();
}

void AnimationTreeModel::removeItem(const QModelIndex& index)
{
    if(!index.isValid())
        return;
    
    QModelIndex parent_index = parent(index);
    if(!parent_index.isValid())
        return;
    
    CompositeAnimation& parent_anim = *static_cast<CompositeAnimation*>(parent_index.internalPointer());
    if(parent_anim.count() < 2)
        return;
    
    beginRemoveRows(parent_index, index.row(), index.row());
    parent_anim.releaseAnimation(index.row());
    endRemoveRows();
}

void AnimationTreeModel::replaceItem(const QModelIndex& index, std::unique_ptr<Animation> replacement)
{
    if(!index.isValid())
        return;

    QModelIndex parent_index = parent(index);
    Animation* added = replacement.get();
    if(parent_index.isValid())
    {
        CompositeAnimation& parent_anim = *static_cast<CompositeAnimation*>(parent_index.internalPointer());
        beginRemoveRows(parent_index, index.row(), index.row());
        parent_anim.swapAnimation(index.row(), std::move(replacement));
        endRemoveRows();
        beginInsertRows(parent_index, index.row(), index.row());
        _parents.clear();
        registerChildren(*static_cast<CompositeAnimation*>(_root.get()));
        endInsertRows();
    }
    else
    {
        if(!replacement)
            replacement = std::make_unique<NullAnimation>(_root->skeleton());
        if(replacement->skeleton() != _root->skeleton())
            return;
        
        beginResetModel();
        _root = std::move(replacement);
        _parents.clear();
        if(CompositeAnimation* as_comp = dynamic_cast<CompositeAnimation*>(added))
            registerChildren(*as_comp);
        endResetModel();
    }    
}

    
QVariant AnimationTreeModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || role != Qt::DisplayRole)
        return QVariant();
    
    return QString(static_cast<Animation*>(index.internalPointer())->name().c_str());
}

QModelIndex AnimationTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if(column)
        return QModelIndex();
    if(!parent.isValid())
    {
        if(row)
            return QModelIndex();
        return createIndex(0, 0, _root.get());
    }

    Animation* parent_anim = static_cast<Animation*>(parent.internalPointer());
    CompositeAnimation* as_comp = dynamic_cast<CompositeAnimation*>(parent_anim);
    if(!as_comp || row >= as_comp->count())
        return QModelIndex();

    return createIndex(row, 0, &(*as_comp)[row]);
}

QModelIndex AnimationTreeModel::parent(const QModelIndex& index) const
{
    auto it = _parents.find(static_cast<Animation*>(index.internalPointer()));
    if(it == _parents.end())
        return QModelIndex();
    
    Animation* parent = it->second;
    it = _parents.find(parent);
    if(it == _parents.end())
        return createIndex(0, 0, parent);
    
    for(int i = 0; i < it->second->count(); ++i)
    {
        if(&(*it->second)[i] == parent)
            return createIndex(i, 0, parent);
    }
    return QModelIndex();
}

int AnimationTreeModel::rowCount(const QModelIndex& parent) const
{
    if(!parent.isValid())
        return 1;
    
    Animation* anim = static_cast<Animation*>(parent.internalPointer());
    if(CompositeAnimation* as_comp = dynamic_cast<CompositeAnimation*>(anim))
        return as_comp->count();
    return 0;
}

int AnimationTreeModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

void AnimationTreeModel::registerChildren(CompositeAnimation& comp)
{
    for(int i = 0; i < comp.count(); ++i)
    {
        Animation* anim = &comp[i];
        _parents[anim] = &comp;
        if(CompositeAnimation* as_comp = dynamic_cast<CompositeAnimation*>(anim))
            registerChildren(*as_comp);
    }
}
