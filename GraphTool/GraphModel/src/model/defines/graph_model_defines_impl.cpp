module;
#include <pch.h>

module graph_model_defines;

static constexpr auto WORLD_POS_OFFSET = static_cast<int>(WORLD_BOUNDS_FIXED_SIZE);

Node::Node(Vector2D worldPos) : m_selected{0}, m_state{0} {
    m_worldPosX = static_cast<uint32_t>(static_cast<int>(worldPos.m_x) + WORLD_POS_OFFSET);
    m_worldPosY = static_cast<uint32_t>(static_cast<int>(worldPos.m_y) + WORLD_POS_OFFSET);

    m_red = m_green = m_blue = 0;
}

BoundingBox2D Node::getBoundingBox(Vector2D worldPos, float radius) {
    return BoundingBox2D{worldPos.m_x - radius, worldPos.m_y - radius, worldPos.m_x + radius,
                         worldPos.m_y + radius};
}

Vector2D Node::getWorldPos() const {
    auto decode = [](int32_t val) { return static_cast<float>(val - WORLD_POS_OFFSET); };
    return {decode(m_worldPosX), decode(m_worldPosY)};
}

bool Node::hasCustomColor() const { return !(m_red == 0 && m_green == 0 && m_blue == 0); }

uint32_t Node::getABGR(int alpha) const {
    constexpr auto expand5 = [](uint8_t v) -> uint8_t { return (v * 255) / 31; };
    constexpr auto expand6 = [](uint8_t v) -> uint8_t { return (v * 255) / 63; };

    const auto r = expand5(static_cast<uint8_t>(m_red));
    const auto g = expand6(static_cast<uint8_t>(m_green));
    const auto b = expand5(static_cast<uint8_t>(m_blue));

    return (static_cast<uint32_t>(alpha) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);
}

void Node::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    m_red = static_cast<uint8_t>((red * 31) / 255);
    m_green = static_cast<uint8_t>((green * 63) / 255);
    m_blue = static_cast<uint8_t>((blue * 31) / 255);
}

void Node::clearColor() { m_red = m_green = m_blue = 0; }

void Node::setState(uint8_t state) {
    if (state >= (1 << NODE_STATE_BITS)) {
        GAPP_THROW("Node state exceeds maximum allowed value");
    }

    m_state = state;
}

uint8_t Node::getState() const { return m_state; }

void Node::markSelected() { m_selected = 1; }

void Node::unmarkSelected() { m_selected = 0; }

bool Node::isSelected() const { return m_selected == 1; }
