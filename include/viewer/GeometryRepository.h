#pragma once

#include <memory>
#include <string>
#include <vector>

class OCCGeometry;

class GeometryRepository {
public:
    explicit GeometryRepository(std::vector<std::shared_ptr<OCCGeometry>>* storage);

    bool existsByName(const std::string& name) const;
    std::shared_ptr<OCCGeometry> findByName(const std::string& name) const;
    void add(const std::shared_ptr<OCCGeometry>& geometry);
    void remove(const std::shared_ptr<OCCGeometry>& geometry);
    void clear();

    const std::vector<std::shared_ptr<OCCGeometry>>& all() const;

private:
    std::vector<std::shared_ptr<OCCGeometry>>* m_storage;
};



