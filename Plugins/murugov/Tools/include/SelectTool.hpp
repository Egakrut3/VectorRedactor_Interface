#pragma once

#include "Interfaces/IDocument.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/IToolManager.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Matrix3F.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <variant>

namespace Murugov
{

namespace Tools
{

constexpr IToolManager::ATool::ID SELECT_TOOL_ID = 5;

class SelectTool final : public IToolManager::ATool
{
public:
    SelectTool(ID id, IDocument& document, IRenderer& renderer, const BoxF& ui_box);
    ~SelectTool() override = default;

    SelectTool(const SelectTool&)            = delete;
    SelectTool(SelectTool&&)                 = delete;
    SelectTool& operator=(const SelectTool&) = delete;
    SelectTool& operator=(SelectTool&&)      = delete;

    ID   id() const override;
    bool processEvent(const IInputPump::Event& event) override;
    void render();

private:
    enum class Mode
    {
        NONE,
        MOVE,
        RESIZE,
        ROTATE,
    };

    static constexpr float HANDLE_HALF_SIZE   = 4.5F;
    static constexpr float ROTATE_HANDLE_DIST = 26.0F;
    static constexpr float MIN_SHAPE_SIZE     = 4.0F;

    Vec2F toLocal(const Vec2F& global_pos) const;
    Vec2F toGlobal(const Vec2F& local_pos) const;

    IDocument::TShape* selectedShape() const;
    void               clearSelection();

    // 8 точек по часовой стрелке:
    // 0: TL, 1: TM, 2: TR, 3: MR, 4: BR, 5: BM, 6: BL, 7: ML
    std::array<Vec2F, 8> shapeHandles(const IDocument::TShape& shape) const;
    Vec2F                rotateHandlePos(const IDocument::TShape& shape) const;

    bool hitTestPoint(const Vec2F& p, const Vec2F& target, float radius) const;
    int  hitTestResizeHandle(const IDocument::TShape& shape, const Vec2F& local_pos) const;

    bool onMouseDown(const Vec2F& global_pos);
    bool onMouseMove(const Vec2F& global_pos);
    bool onMouseUp();
    bool onKeyDown(const IInputPump::EventKeyboard& key);

private:
    ID         id_;
    IDocument& document_;
    IRenderer& renderer_;
    BoxF       ui_box_;

    /* NOTE: The class stores an `std::optional<TObject::ID> selected_id_` rather than a raw pointer `TShape*`.
             If a shape is deleted by another tool or command, a raw pointer would become a dangling pointer, leading to a crash (segfault).
    */
    std::optional<TObject::ID> selected_id_;
    Mode                       mode_ = Mode::NONE;

    Vec2F    prev_mouse_local_;
    Vec2F    fixed_anchor_; // Противоположный угол или середина стороны в координатах документа
    Vec2F    start_size_;
    Matrix3F start_transform_ {Matrix3F::identity()};
    int      active_handle_idx_ = -1;
}; // class SelectTool

/* ====================== Implementation ====================== */

inline SelectTool::SelectTool(ID id, IDocument& document, IRenderer& renderer, const BoxF& ui_box) :
    id_(id), document_(document), renderer_(renderer), ui_box_(ui_box)
{
}

inline IToolManager::ATool::ID SelectTool::id() const
{
    return id_;
}

inline Vec2F SelectTool::toLocal(const Vec2F& global_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapGlobalToLocal(global_pos);
    }
    return global_pos;
}

inline Vec2F SelectTool::toGlobal(const Vec2F& local_pos) const
{
    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        return node->mapLocalToGlobal(local_pos);
    }
    return local_pos;
}

inline IDocument::TShape* SelectTool::selectedShape() const
{
    if (!selected_id_.has_value())
    {
        return nullptr;
    }

    if (const auto* node = dynamic_cast<const TObject*>(&document_))
    {
        for (const auto& child : node->children())
        {
            if (child && child->id() == *selected_id_)
            {
                return dynamic_cast<IDocument::TShape*>(child.get());
            }
        }
    }

    return nullptr;
}

inline void SelectTool::clearSelection()
{
    selected_id_.reset();
    mode_              = Mode::NONE;
    active_handle_idx_ = -1;
}

inline std::array<Vec2F, 8> SelectTool::shapeHandles(const IDocument::TShape& shape) const
{
    // Матрица фигуры переводит единичный квадрат [-0.5; 0.5] в координаты документа
    const Matrix3F& m = shape.transform();

    return {
        m.transformPoint({-0.5F, -0.5F}), // 0: Top-Left
        m.transformPoint({ 0.0F, -0.5F}), // 1: Top-Middle
        m.transformPoint({+0.5F, -0.5F}), // 2: Top-Right
        m.transformPoint({+0.5F,  0.0F}), // 3: Middle-Right
        m.transformPoint({+0.5F, +0.5F}), // 4: Bottom-Right
        m.transformPoint({ 0.0F, +0.5F}), // 5: Bottom-Middle
        m.transformPoint({-0.5F, +0.5F}), // 6: Bottom-Left
        m.transformPoint({-0.5F,  0.0F}), // 7: Middle-Left
    };
}

inline Vec2F SelectTool::rotateHandlePos(const IDocument::TShape& shape) const
{
    const float h = std::abs(shape.scaling().y());
    if (h <= 1e-4F)
    {
        return shape.translation();
    }

    // Сдвиг вверх на ROTATE_HANDLE_DIST пикселей в нормализованных координатах фигуры
    const float norm_offset_y = -0.5F - (ROTATE_HANDLE_DIST / h);
    return shape.transform().transformPoint({0.0F, norm_offset_y});
}

inline bool SelectTool::hitTestPoint(const Vec2F& p, const Vec2F& target, float radius) const
{
    return std::abs(p.x() - target.x()) <= radius && std::abs(p.y() - target.y()) <= radius;
}

inline int SelectTool::hitTestResizeHandle(const IDocument::TShape& shape, const Vec2F& local_pos) const
{
    const auto handles = shapeHandles(shape);
    for (int i = 0; i < 8; ++i)
    {
        if (hitTestPoint(local_pos, handles[i], HANDLE_HALF_SIZE + 3.0F))
        {
            return i;
        }
    }
    return -1;
}

inline bool SelectTool::processEvent(const IInputPump::Event& event)
{
    if (const auto* key = std::get_if<IInputPump::EventKeyboard>(&event))
    {
        if (key->type() == IInputPump::EventKeyboard::Type::KEY_DOWN)
        {
            return onKeyDown(*key);
        }
        return false;
    }

    const auto* mouse = std::get_if<IInputPump::EventMouse>(&event);
    if (mouse == nullptr)
    {
        return false;
    }

    using enum IInputPump::EventMouse::Type;
    switch (mouse->type())
    {
        case LEFT_BUTTON_PRESSING:
            return onMouseDown(mouse->cursorPos());

        case MOTION:
            return onMouseMove(mouse->cursorPos());

        case LEFT_BUTTON_RELEASING:
            return onMouseUp();

        default:
            return false;
    }
}

inline bool SelectTool::onKeyDown(const IInputPump::EventKeyboard& key)
{
    using Key = IInputPump::EventKeyboard::Key;

    if (key.key() == Key::ESC)
    {
        if (selected_id_.has_value())
        {
            clearSelection();
            return true;
        }
        return false;
    }

    if (key.key() == Key::DEL || key.key() == Key::BACKSPACE)
    {
        if (selected_id_.has_value())
        {
            document_.removeShape(*selected_id_);
            clearSelection();
            return true;
        }
    }

    return false;
}

inline bool SelectTool::onMouseDown(const Vec2F& global_pos)
{
    if (ui_box_.contains(global_pos))
    {
        return false;
    }

    const Vec2F local_pos = toLocal(global_pos);

    if (IDocument::TShape* shape = selectedShape())
    {
        // 1. Ручка вращения
        if (hitTestPoint(local_pos, rotateHandlePos(*shape), HANDLE_HALF_SIZE + 4.0F))
        {
            prev_mouse_local_ = local_pos;
            start_transform_  = shape->transform();
            mode_             = Mode::ROTATE;
            return true;
        }

        // 2. Любой из 8 маркеров ресайза
        const int handle_idx = hitTestResizeHandle(*shape, local_pos);
        if (handle_idx != -1)
        {
            const auto handles = shapeHandles(*shape);
            active_handle_idx_ = handle_idx;
            fixed_anchor_      = handles[(handle_idx + 4) % 8];
            start_size_        = shape->scaling();
            mode_              = Mode::RESIZE;
            return true;
        }
    }

    // 3. Выбор фигуры
    const std::optional<TObject::ID> hit = document_.hitTest(global_pos);
    if (hit.has_value())
    {
        selected_id_      = hit;
        prev_mouse_local_ = local_pos;
        mode_             = Mode::MOVE;
        return true;
    }

    clearSelection();
    return true;
}

inline bool SelectTool::onMouseMove(const Vec2F& global_pos)
{
    if (mode_ == Mode::NONE)
    {
        return false;
    }

    IDocument::TShape* shape = selectedShape();
    if (shape == nullptr)
    {
        clearSelection();
        return false;
    }

    const Vec2F local_pos = toLocal(global_pos);

    if (mode_ == Mode::MOVE)
    {
        const Vec2F delta = local_pos - prev_mouse_local_;
        shape->translate(delta);
        prev_mouse_local_ = local_pos;
        return true;
    }

    if (mode_ == Mode::ROTATE)
    {
        const Vec2F center   = shape->translation();
        const Vec2F prev_vec = prev_mouse_local_ - center;
        const Vec2F curr_vec = local_pos - center;

        if (prev_vec.len() > 1e-3F && curr_vec.len() > 1e-3F)
        {
            const float delta_angle =
                std::atan2(curr_vec.y(), curr_vec.x()) - std::atan2(prev_vec.y(), prev_vec.x());

            shape->rotate(delta_angle);
            prev_mouse_local_ = local_pos;
        }
        return true;
    }

    if (mode_ == Mode::RESIZE)
    {
        const float angle      = shape->rotation();
        const Vec2F diag_local = Matrix3F::rotation(-angle).transformPoint(local_pos - fixed_anchor_);

        float new_w    = start_size_.x();
        float new_h    = start_size_.y();
        float offset_x = 0.0F;
        float offset_y = 0.0F;

        const bool resize_x = (active_handle_idx_ % 2 == 0) || (active_handle_idx_ == 3) || (active_handle_idx_ == 7);
        const bool resize_y = (active_handle_idx_ % 2 == 0) || (active_handle_idx_ == 1) || (active_handle_idx_ == 5);

        if (resize_x)
        {
            new_w    = std::max(std::abs(diag_local.x()), MIN_SHAPE_SIZE);
            offset_x = (diag_local.x() >= 0.0F ? new_w : -new_w) * 0.5F;
        }

        if (resize_y)
        {
            new_h    = std::max(std::abs(diag_local.y()), MIN_SHAPE_SIZE);
            offset_y = (diag_local.y() >= 0.0F ? new_h : -new_h) * 0.5F;
        }

        const Vec2F new_center = fixed_anchor_ + Matrix3F::rotation(angle).transformPoint({offset_x, offset_y});
        shape->setScaling({new_w, new_h});
        shape->setTranslation(new_center);

        return true;
    }


    return false;
}
inline bool SelectTool::onMouseUp()
{
    if (mode_ == Mode::NONE)
    {
        return false;
    }
    
    mode_              = Mode::NONE;
    active_handle_idx_ = -1;
    
    return true;
}

inline void SelectTool::render()
{
    IDocument::TShape* shape = selectedShape();
    if (shape == nullptr)
    {
        return;
    }
    
    const IRenderer::Color accent = IRenderer::Color::Blue;
    const IRenderer::Color fill   = IRenderer::Color::White;
    
    const auto  handles = shapeHandles(*shape);
    const Vec2F top_mid = toGlobal(handles[1]);
    const Vec2F rot_pt  = toGlobal(rotateHandlePos(*shape));
    
    for (int i = 0; i < 4; ++i)
    {
        const Vec2F c1 = toGlobal(handles[i * 2]);
        const Vec2F c2 = toGlobal(handles[((i + 1) % 4) * 2]);
        renderer_.renderLine(accent, c1, c2);
    }
    
    renderer_.renderLine(accent, top_mid, rot_pt);
    
    const BoxF rot_box(rot_pt.x() - HANDLE_HALF_SIZE, rot_pt.y() - HANDLE_HALF_SIZE,
    rot_pt.x() + HANDLE_HALF_SIZE, rot_pt.y() + HANDLE_HALF_SIZE);
    renderer_.renderEllipse(fill, rot_box);
    renderer_.renderEllipseLines(accent, rot_box);
    
    for (const Vec2F& h : handles)
    {
        const Vec2F g = toGlobal(h);
        const BoxF  h_box(g.x() - HANDLE_HALF_SIZE, g.y() - HANDLE_HALF_SIZE,
        g.x() + HANDLE_HALF_SIZE, g.y() + HANDLE_HALF_SIZE);
        renderer_.renderRect(fill, h_box);
        renderer_.renderRectLines(accent, h_box);
    }
}

}; // namespace Tools

}; // namespace Murugov