#pragma once
#include <cstdint>
#include <exception>
#include <sstream>
#include <string>

class CException : public std::exception {
   public:
    CException(uint32_t line, const char* file);
    CException(uint32_t line, const char* file, std::string type,
	       std::string originalString);
    const char* what() const noexcept override;

    uint32_t GetLine() const noexcept;
    const char* GetFile() const noexcept;

    virtual std::string GetType() const noexcept;
    virtual std::string GetOriginalString() const noexcept;

   private:
    uint32_t line;
    const char* file;

   protected:
    std::string type, originalString;
    mutable std::string whatBuffer;
};

