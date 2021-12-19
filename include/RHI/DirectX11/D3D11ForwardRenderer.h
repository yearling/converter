#pragma once
namespace SKwai {
class D3D11ForwardRenderer  {
  public:
    void BeginFrame();
    void EndFrame();

  private:
    bool has_render_target_ = false;

};
}  // namespace SKwai