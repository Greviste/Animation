#ifndef ANIMATIONLISTMODEL_H
#define ANIMATIONLISTMODEL_H

#include <QAbstractListModel>
#include <vector>
#include <memory>
#include "Animation.h"


class AnimationListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    void addAnimation(std::unique_ptr<SimpleAnimation> anim);
    void clear();
    SimpleAnimation& operator[](int i) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    std::vector<std::unique_ptr<SimpleAnimation>> _animations;
};

#endif
