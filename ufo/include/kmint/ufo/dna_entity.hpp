#ifndef PROJECT_DNA_ENTITY_HPP
#define PROJECT_DNA_ENTITY_HPP

#include <memory>
#include "kmint/ufo/dna_string.hpp"

namespace kmint::ufo {

    class dna_entity
    {
    public:
        dna_entity();
        void setGeneticAttributes(dna_string &a);
        dna_string getGeneticAttributes() const;
    protected:
        dna_string geneticAttributes;
    };

}

#endif //PROJECT_DNA_ENTITY_HPP
