#ifndef ANIMATIONTREEMODEL_H
#define ANIMATIONTREEMODEL_H

#include <QAbstractItemModel>
#include <memory>
#include <unordered_map>
#include "Animation.h"


class AnimationTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    AnimationTreeModel(std::shared_ptr<const Skeleton> skeleton);
    Animation& getAnimation(const QModelIndex& index = QModelIndex()) const;
    void addChild(const QModelIndex& index);
    void removeItem(const QModelIndex& index);
    void replaceItem(const QModelIndex& index, std::unique_ptr<Animation> replacement);
    template<typename T>
    void replaceItem(const QModelIndex& index)
    {
        replaceItem(index, std::make_unique<T>(_root->skeleton()));
    }

    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
    void registerChildren(CompositeAnimation& comp);
    
    std::unique_ptr<Animation> _root;
    std::unordered_map<Animation*, CompositeAnimation*> _parents;
};

#endif
