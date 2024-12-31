#pragma once
#include "decoration.h"
#include "vertex.h"
#include <vector>

namespace ms {

class VertexDecoration : public Decoration {
public:
    VertexDecoration(const std::string& fillStyle, const std::string& strokeStyle,
                    const std::function<void()>& onChange = nullptr);

    void set(const std::string& name, float value) override;
    void set(const std::string& name, bool value) override;
    void set(const std::string& name, const std::string& value) override;
    float getFloat(const std::string& name) const override;
    bool getBool(const std::string& name) const override;
    std::string getString(const std::string& name) const override;

    void draw(Context* context, 
             const std::function<Vec2(const Vec2&)>& transform) override;

    // Vertex management
    void addVertex(Vertex* vertex);
    void removeVertex(Vertex* vertex);
    const std::vector<Vertex*>& getVertices() const;
    void clearVertices();

private:
    std::vector<Vertex*> vertices;
    std::map<std::string, float> floatProperties;
    std::map<std::string, bool> boolProperties;
    std::map<std::string, std::string> stringProperties;

    void drawVertex(Context* context, 
                   const Vec2& position,
                   const std::function<Vec2(const Vec2&)>& transform);
};

} // namespace ms 