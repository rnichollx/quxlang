// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef STRING_HPP
#define STRING_HPP
#include <vector>

namespace rpnx
{
    class string
    {
        std::vector< char > data;

    public:
        string() = default;

        string(const char* str)
        {
            while (*str)
            {
                data.push_back(*str);
                str++;
            }
            data.push_back(0);
        }

        template <typename It>
        string(It begin, It end)
        {
            while (begin != end)
            {
                data.push_back(*begin);
                begin++;
            }
            data.push_back(0);
        }


        char& operator[](std::size_t index)
        {
            return data[index];
        }

        char const& operator[](std::size_t index) const
        {
            return data[index];
        }

        std::size_t size() const
        {
            return data.size();
        }

        std::size_t capacity() const
        {
            return data.capacity();
        }

        operator std::string() const
        {
            return std::string(data.data());
        }


        std::strong_ordering operator <=>(const string& other) const
        {
            if (data.size() < other.data.size())
                return std::strong_ordering::less;
            if (data.size() > other.data.size())
                return std::strong_ordering::greater;
            for (std::size_t i = 0; i < data.size() && i < other.data.size(); i++)
            {
                if (data[i] < other.data[i])
                    return std::strong_ordering::less;
                if (data[i] > other.data[i])
                    return std::strong_ordering::greater;
            }

            return std::strong_ordering::equal;
        }


    };
}

#endif //STRING_HPP