#pragma once

// libs
// FTXUI includes
#include "ftxui/component/captured_mouse.hpp" // for CapturedMouse
#include "ftxui/component/component.hpp"      // for Make, Slider
#include "ftxui/component/component_base.hpp" // for ComponentBase
#include "ftxui/component/event.hpp" // for Event, Event::ArrowDown, Event::ArrowLeft, Event::ArrowRight, Event::ArrowUp
#include "ftxui/component/mouse.hpp" // for Mouse, Mouse::Left, Mouse::Pressed, Mouse::Released
#include "ftxui/component/screen_interactive.hpp" // for Component
#include "ftxui/dom/elements.hpp" // for operator|, text, Element, xflex, hbox, color, underlined, reflect, Decorator, dim, vcenter, focus, nothing, select, yflex, gaugeDirection
#include "ftxui/screen/box.hpp"   // for Box
#include "ftxui/screen/color.hpp" // for Color, Color::GrayDark, Color::White
#include "ftxui/util/ref.hpp"     // for ConstRef, Ref, ConstStringRef

// std
#include <algorithm>  // for max, min
#include <functional> // for function
#include <string>     // for allocator
#include <utility>    // for move

namespace ftxui {

namespace util {

// Similar to std::clamp, but allow hi to be lower than lo.
template <class T>
constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
  return v < lo ? lo : hi < v ? hi : v;
}

} // namespace util

namespace {

Decorator flexDirection(Direction direction) {
  switch (direction) {
  case Direction::Up:
  case Direction::Down:
    return yflex;
  case Direction::Left:
  case Direction::Right:
    return xflex;
  }
  return xflex; // NOT_REACHED()
}

// @brief Option for the `SliderWithCallback` component.
// @ingroup component
template <typename T> struct SliderWithCallbackOption {
  std::function<void(T)> callback;
  Ref<T> value;
  ConstRef<T> min = T(0);
  ConstRef<T> max = T(100);
  ConstRef<T> increment = (max() - min()) / 20;
  Direction direction = Direction::Right;
  Color color_active = Color::White;
  Color color_inactive = Color::GrayDark;
};

template <class T> class SliderBase : public ComponentBase {
public:
  explicit SliderBase(SliderOption<T> options)
      : value_(options.value), min_(options.min), max_(options.max),
        increment_(options.increment), options_(options) {}

  Element Render() override {
    auto gauge_color = Focused() ? color(options_.color_active)
                                 : color(options_.color_inactive);
    const float percent = float(value_() - min_()) / float(max_() - min_());
    return gaugeDirection(percent, options_.direction) |
           flexDirection(options_.direction) | reflect(gauge_box_) |
           gauge_color;
  }

  void OnLeft() {
    switch (options_.direction) {
    case Direction::Right:
      value_() -= increment_();
      break;
    case Direction::Left:
      value_() += increment_();
      break;
    case Direction::Up:
    case Direction::Down:
      break;
    }
  }

  void OnRight() {
    switch (options_.direction) {
    case Direction::Right:
      value_() += increment_();
      break;
    case Direction::Left:
      value_() -= increment_();
      break;
    case Direction::Up:
    case Direction::Down:
      break;
    }
  }

  void OnUp() {
    switch (options_.direction) {
    case Direction::Up:
      value_() -= increment_();
      break;
    case Direction::Down:
      value_() += increment_();
      break;
    case Direction::Left:
    case Direction::Right:
      break;
    }
  }

  void OnDown() {
    switch (options_.direction) {
    case Direction::Down:
      value_() -= increment_();
      break;
    case Direction::Up:
      value_() += increment_();
      break;
    case Direction::Left:
    case Direction::Right:
      break;
    }
  }

  bool OnEvent(Event event) final {
    if (event.is_mouse()) {
      return OnMouseEvent(event);
    }

    T old_value = value_();
    if (event == Event::ArrowLeft || event == Event::Character('h')) {
      OnLeft();
    }
    if (event == Event::ArrowRight || event == Event::Character('l')) {
      OnRight();
    }
    if (event == Event::ArrowUp || event == Event::Character('k')) {
      OnDown();
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
      OnUp();
    }

    value_() = util::clamp(value_(), min_(), max_());
    if (old_value != value_()) {
      return true;
    }

    return ComponentBase::OnEvent(event);
  }

  bool OnMouseEvent(Event event) {
    if (captured_mouse_) {
      if (event.mouse().motion == Mouse::Released) {
        captured_mouse_ = nullptr;
        return true;
      }

      switch (options_.direction) {
      case Direction::Right: {
        value_() = min_() + (event.mouse().x - gauge_box_.x_min) *
                                (max_() - min_()) /
                                (gauge_box_.x_max - gauge_box_.x_min);
        break;
      }
      case Direction::Left: {
        value_() = max_() - (event.mouse().x - gauge_box_.x_min) *
                                (max_() - min_()) /
                                (gauge_box_.x_max - gauge_box_.x_min);
        break;
      }
      case Direction::Down: {
        value_() = min_() + (event.mouse().y - gauge_box_.y_min) *
                                (max_() - min_()) /
                                (gauge_box_.y_max - gauge_box_.y_min);
        break;
      }
      case Direction::Up: {
        value_() = max_() - (event.mouse().y - gauge_box_.y_min) *
                                (max_() - min_()) /
                                (gauge_box_.y_max - gauge_box_.y_min);
        break;
      }
      }
      value_() = std::max(min_(), std::min(max_(), value_()));
      return true;
    }

    if (event.mouse().button != Mouse::Left) {
      return false;
    }
    if (event.mouse().motion != Mouse::Pressed) {
      return false;
    }

    if (!gauge_box_.Contain(event.mouse().x, event.mouse().y)) {
      return false;
    }

    captured_mouse_ = CaptureMouse(event);

    if (captured_mouse_) {
      TakeFocus();
      return true;
    }

    return false;
  }

  bool Focusable() const final { return true; }

private:
  Ref<T> value_;
  ConstRef<T> min_;
  ConstRef<T> max_;
  ConstRef<T> increment_;
  SliderOption<T> options_;
  Box gauge_box_;
  CapturedMouse captured_mouse_;
};

class SliderWithLabel : public ComponentBase {
public:
  SliderWithLabel(const Element label, Component inner)
      : label_(std::move(label)) {
    Add(std::move(inner));
    SetActiveChild(ChildAt(0));
  }

private:
  bool OnEvent(Event event) final {
    if (ComponentBase::OnEvent(event)) {
      return true;
    }

    if (!event.is_mouse()) {
      return false;
    }

    mouse_hover_ = box_.Contain(event.mouse().x, event.mouse().y);

    if (!mouse_hover_) {
      return false;
    }

    if (!CaptureMouse(event)) {
      return false;
    }

    return true;
  }

  Element Render() override {
    auto focus_management = Focused() ? focus : Active() ? select : nothing;
    auto gauge_color = (Focused() || mouse_hover_) ? color(Color::White)
                                                   : color(Color::GrayDark);
    return hbox({
               label_ | dim | vcenter,
               hbox({
                   text("["),
                   ComponentBase::Render() | underlined,
                   text("]"),
               }) | xflex,
           }) |
           gauge_color | xflex | reflect(box_) | focus_management;
  }

  const Element label_;
  Box box_;
  bool mouse_hover_ = false;
};

template <class T> class SliderWithCallback : public ComponentBase {
public:
  explicit SliderWithCallback(SliderWithCallbackOption<T> options)
      : callback_(options.callback), min_(options.min), max_(options.max),
        increment_(options.increment), options_(options) {
    SetValue(options.value());
  }

  Element Render() override {
    auto gauge_color = Focused() ? color(options_.color_active)
                                 : color(options_.color_inactive);
    const float percent = float(value_() - min_()) / float(max_() - min_());
    return gaugeDirection(percent, options_.direction) |
           flexDirection(options_.direction) | reflect(gauge_box_) |
           gauge_color;
  }

  void OnLeft() {
    switch (options_.direction) {
    case Direction::Right:
      SetValue(value_() - increment_());
      break;
    case Direction::Left:
      SetValue(value_() + increment_());
      break;
    case Direction::Up:
    case Direction::Down:
      break;
    }
  }

  void OnRight() {
    switch (options_.direction) {
    case Direction::Right:
      SetValue(value_() + increment_());
      break;
    case Direction::Left:
      SetValue(value_() - increment_());
      break;
    case Direction::Up:
    case Direction::Down:
      break;
    }
  }

  void OnUp() {
    switch (options_.direction) {
    case Direction::Up:
      SetValue(value_() - increment_());
      break;
    case Direction::Down:
      SetValue(value_() + increment_());
      break;
    case Direction::Left:
    case Direction::Right:
      break;
    }
  }

  void OnDown() {
    switch (options_.direction) {
    case Direction::Down:
      SetValue(value_() - increment_());
      break;
    case Direction::Up:
      SetValue(value_() + increment_());
      break;
    case Direction::Left:
    case Direction::Right:
      break;
    }
  }

  bool OnEvent(Event event) final {
    if (event.is_mouse()) {
      return OnMouseEvent(event);
    }

    T old_value = value_();
    if (event == Event::ArrowLeft || event == Event::Character('h')) {
      OnLeft();
    }
    if (event == Event::ArrowRight || event == Event::Character('l')) {
      OnRight();
    }
    if (event == Event::ArrowUp || event == Event::Character('k')) {
      OnDown();
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
      OnUp();
    }

    SetValue(util::clamp(value_(), min_(), max_()));
    if (old_value != value_()) {
      return true;
    }

    return ComponentBase::OnEvent(event);
  }

  bool OnMouseEvent(Event event) {
    if (captured_mouse_) {
      if (event.mouse().motion == Mouse::Released) {
        captured_mouse_ = nullptr;
        return true;
      }

      switch (options_.direction) {
      case Direction::Right: {
        SetValue(min_() + (event.mouse().x - gauge_box_.x_min) *
                              (max_() - min_()) /
                              (gauge_box_.x_max - gauge_box_.x_min));

        break;
      }
      case Direction::Left: {
        SetValue(max_() - (event.mouse().x - gauge_box_.x_min) *
                              (max_() - min_()) /
                              (gauge_box_.x_max - gauge_box_.x_min));
        break;
      }
      case Direction::Down: {
        SetValue(min_() + (event.mouse().y - gauge_box_.y_min) *
                              (max_() - min_()) /
                              (gauge_box_.y_max - gauge_box_.y_min));
        break;
      }
      case Direction::Up: {
        SetValue(max_() - (event.mouse().y - gauge_box_.y_min) *
                              (max_() - min_()) /
                              (gauge_box_.y_max - gauge_box_.y_min));
        break;
      }
      }

      SetValue(std::max(min_(), std::min(max_(), value_())));
      return true;
    }

    if (event.mouse().button != Mouse::Left) {
      return false;
    }
    if (event.mouse().motion != Mouse::Pressed) {
      return false;
    }

    if (!gauge_box_.Contain(event.mouse().x, event.mouse().y)) {
      return false;
    }

    captured_mouse_ = CaptureMouse(event);

    if (captured_mouse_) {
      TakeFocus();
      return true;
    }

    return false;
  }

  bool Focusable() const final { return true; }

  void SetValue(Ref<T> val) {
    value_() = util::clamp(val(), min_(), max_());
    callback_(value_());
  }

private:
  std::function<void(T)> callback_;
  Ref<T> value_;
  ConstRef<T> min_;
  ConstRef<T> max_;
  ConstRef<T> increment_;
  SliderWithCallbackOption<T> options_;
  Box gauge_box_;
  CapturedMouse captured_mouse_;
};

} // namespace

// Base slider with label
template <typename T>
Component Slider(const Element label, SliderOption<T> options) {
  auto slider = Make<SliderBase<T>>(options);
  return Make<SliderWithLabel>(std::move(label), slider);
}

// Slider with callback
template <typename T> Component Slider(SliderWithCallbackOption<T> options) {
  return Make<SliderWithCallback<T>>(options);
}

// Slider with label and callback
template <typename T>
Component Slider(const Element label, SliderWithCallbackOption<T> options) {
  auto slider = Make<SliderWithCallback<T>>(options);
  return Make<SliderWithLabel>(std::move(label), slider);
}

} // namespace ftxui