#pragma once

struct AlgorithmLabel {
    explicit AlgorithmLabel(QGraphicsScene* scene, QPointF pos,
                            std::function<void(QGraphicsSimpleTextItem*)> computeFunc);
    ~AlgorithmLabel();

    AlgorithmLabel(const AlgorithmLabel&) = delete;
    AlgorithmLabel& operator=(const AlgorithmLabel&) = delete;

    AlgorithmLabel(AlgorithmLabel&& other) = delete;
    AlgorithmLabel& operator=(AlgorithmLabel&& other) = delete;

    void compute() const;

   private:
    QGraphicsScene* m_scene;
    QGraphicsSimpleTextItem* m_label;
    std::function<void(QGraphicsSimpleTextItem*)> m_compute;
};
