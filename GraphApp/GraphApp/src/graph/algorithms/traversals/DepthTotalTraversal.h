#pragma once

#include "DepthTraversal.h"

class DepthTotalTraversal : public DepthTraversal {
    Q_OBJECT

   public:
    DepthTotalTraversal(Graph* parent);

    void showPseudocode() override;

   protected:
    bool stepOnce() override;
    void stepAll() override;

    virtual bool pickAnotherNode();
};
