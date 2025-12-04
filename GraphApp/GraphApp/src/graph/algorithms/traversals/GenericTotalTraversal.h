#pragma once

#include "GenericTraversal.h"

class GenericTotalTraversal : public GenericTraversal {
    Q_OBJECT

   public:
    GenericTotalTraversal(Graph* parent);

    void showPseudocode() override;

   protected:
    bool stepOnce() override;
    void stepAll() override;

   private:
    void pickAnotherNode();
};
