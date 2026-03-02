module;
#include <pch.h>

module graph_model_defines;

static constexpr auto WORLD_POS_OFFSET = static_cast<int>(WORLD_BOUNDS_FIXED_SIZE);

Node::Node(Vector2D worldPos) : m_selected{0} {
    m_worldPosX = static_cast<uint32_t>(static_cast<int>(worldPos.m_x) + WORLD_POS_OFFSET);
    m_worldPosY = static_cast<uint32_t>(static_cast<int>(worldPos.m_y) + WORLD_POS_OFFSET);

    setColor(xorshift32(), xorshift32(), xorshift32());
}

BoundingBox2D Node::getBoundingBox(Vector2D worldPos) {
    return BoundingBox2D{worldPos.m_x - NODE_RADIUS, worldPos.m_y - NODE_RADIUS,
                         worldPos.m_x + NODE_RADIUS, worldPos.m_y + NODE_RADIUS};
}

Vector2D Node::getWorldPos() const {
    auto decode = [](int32_t val) { return static_cast<float>(val - WORLD_POS_OFFSET); };
    return {decode(m_worldPosX), decode(m_worldPosY)};
}

bool Node::hasCustomColor() const { return !(m_red == 0 && m_green == 0 && m_blue == 0); }

uint32_t Node::getABGR(int alpha) const {
    auto dequantize = [](uint8_t val) -> uint8_t { return val * 180 / 127 + 30; };

    const auto r = dequantize(m_red);
    const auto g = dequantize(m_green);
    const auto b = dequantize(m_blue);

    return (static_cast<uint32_t>(alpha) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);
}

void Node::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    static constexpr auto buildLUT = []() {
        std::array<uint8_t, 256> lut{};
        for (int i = 0; i < 256; ++i) {
            lut[i] = (std::clamp(i, 30, 210) - 30) * 127 / 180;
        }
        return lut;
    };

    static constexpr auto lut = buildLUT();

    m_red = lut[red];
    m_green = lut[green];
    m_blue = lut[blue];
}

void Node::markSelected() { m_selected = 1; }

void Node::unmarkSelected() { m_selected = 0; }

bool Node::isSelected() const { return m_selected == 1; }
