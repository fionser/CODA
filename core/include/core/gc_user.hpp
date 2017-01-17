#ifndef CORE_GC_USER_HPP
#define CORE_GC_USER_HPP
#include <string>
#include <memory>
namespace plugin {
namespace gc {
class GCUser {
public:
    GCUser(const std::string &inputFile) : inputFile(inputFile) {}
    ~GCUser() {}
    virtual bool run(const std::string &outputFile) const = 0;
protected:
    std::string inputFile;
};

// circut generator
// stands for the server
class Alice : public GCUser {
public:
    Alice(const std::string &inputFile);

    ~Alice() {}

    bool run(const std::string &outputFile) const override;
private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};

// circut evaluator
// stands for the analyst
class Bob: public GCUser {
public:
    Bob(const std::string &ipAddr,
        const long port,
        const std::string &inputFile);

    ~Bob() {}

    bool run(const std::string &outputFile) const override;
private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};
} // namespace gc
} // namespace plugin
#endif // CORE_GC_USER_HPP
