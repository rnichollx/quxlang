// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL


#include <filesystem>

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        return 1;
    }

    std::filesystem::path input = argv[1];
    std::filesystem::path output = argv[2];
}