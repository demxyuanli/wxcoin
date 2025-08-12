#include "viewer/GeometryRepository.h"
#include "OCCGeometry.h"

#include <algorithm>

GeometryRepository::GeometryRepository(std::vector<std::shared_ptr<OCCGeometry>>* storage)
    : m_storage(storage) {}

bool GeometryRepository::existsByName(const std::string& name) const {
    if (!m_storage) return false;
    return std::any_of(m_storage->begin(), m_storage->end(), [&](const std::shared_ptr<OCCGeometry>& g){
        return g && g->getName() == name;
    });
}

std::shared_ptr<OCCGeometry> GeometryRepository::findByName(const std::string& name) const {
    if (!m_storage) return nullptr;
    auto it = std::find_if(m_storage->begin(), m_storage->end(), [&](const std::shared_ptr<OCCGeometry>& g){
        return g && g->getName() == name;
    });
    return it != m_storage->end() ? *it : nullptr;
}

void GeometryRepository::add(const std::shared_ptr<OCCGeometry>& geometry) {
    if (!m_storage || !geometry) return;
    m_storage->push_back(geometry);
}

void GeometryRepository::remove(const std::shared_ptr<OCCGeometry>& geometry) {
    if (!m_storage || !geometry) return;
    auto it = std::find(m_storage->begin(), m_storage->end(), geometry);
    if (it != m_storage->end()) m_storage->erase(it);
}

void GeometryRepository::clear() {
    if (!m_storage) return;
    m_storage->clear();
}

const std::vector<std::shared_ptr<OCCGeometry>>& GeometryRepository::all() const {
    static const std::vector<std::shared_ptr<OCCGeometry>> empty;
    return m_storage ? *m_storage : empty;
}


