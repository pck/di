//
// Copyright (c) 2012-2019 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//<-
#include <cassert>
#include <string>
//->
#include <boost/di.hpp>

namespace di = boost::di;

//<-
struct renderer {
  int device;
};

class iview {
 public:
  virtual ~iview() noexcept = default;
  virtual void update() = 0;
};

class gui_view : public iview {
 public:
  gui_view(std::string /*title*/, const renderer& r) { assert(42 == r.device); }
  void update() override {}
};

class text_view : public iview {
 public:
  void update() override {}
};

class model {};

//<-
class controller {
 public:
  controller(model&, iview& v) { assert(dynamic_cast<gui_view*>(&v)); }
};
//->

class user {};

class app {
 public:
  app(controller&, user&) {}
};
//->

int main() {
  auto use_gui_view = true;

  // clang-format off
  auto injector = di::make_injector(
    di::bind<iview>().to([&](const auto& injector) -> iview& {
      if (use_gui_view)
        return injector.template create<gui_view&>();
      else
        return injector.template create<text_view&>();
    })
  , di::bind<int>().to(42) // renderer device
  );
  // clang-format on

  injector.create<app>();
}
