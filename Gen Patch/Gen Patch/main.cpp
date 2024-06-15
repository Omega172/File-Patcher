#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <openssl/sha.h> // For SHA256 hashing
#include "CommandLineParser/CommandLineParser.h"

bool g_bVerbose = false;

// Function to calculate SHA256 hash of a file
std::string CalculateFileHash(const std::filesystem::path& filePath) {
    std::ifstream fileStream(filePath, std::ios::binary);
    if (!fileStream) {
        std::cerr << "Failed to open file for hashing: " << filePath.string() << std::endl;
        return "";
    }

    // Use SHA256 to calculate the hash
    constexpr int bufferSize = 4096;
    unsigned char buffer[bufferSize];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    while (fileStream.good()) {
        fileStream.read(reinterpret_cast<char*>(buffer), bufferSize);
        auto bytesRead = fileStream.gcount();
        SHA256_Update(&sha256, buffer, bytesRead);
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    // Convert the hash to a hexadecimal string
    std::stringstream hashStream;
    hashStream << std::hex << std::setfill('0');
    for (auto byte : hash) {
        hashStream << std::setw(2) << static_cast<int>(byte);
    }

    return hashStream.str();
}

// Function to generate a patch file
void GeneratePatchFile(const std::filesystem::path& pathOriginalFile,
    const std::filesystem::path& pathModifiedFile,
    const std::filesystem::path& pathPatchFile) {
    // Calculate hashes of original and modified files
    std::string originalHash = CalculateFileHash(pathOriginalFile);

    if (originalHash.empty()) {
        std::cerr << "Failed to generate patch: Failed to calculate file hashes." << std::endl;
        return;
    }

    // Open patch file for writing
    std::ofstream patchFile(pathPatchFile);
    if (!patchFile) {
        std::cerr << "Failed to open patch file for writing: " << pathPatchFile.string() << std::endl;
        return;
    }

    // Write header information to patch file
    patchFile << "PATCH FILE" << std::endl;
    patchFile << "Original File Hash: " << originalHash << std::endl;
    patchFile << "--------------------------------------------------" << std::endl;

    // Compare byte differences between original and modified files
    std::ifstream originalFile(pathOriginalFile, std::ios::binary);
    std::ifstream modifiedFile(pathModifiedFile, std::ios::binary);

    if (!originalFile || !modifiedFile) {
        std::cerr << "Failed to open files for comparison: " << pathOriginalFile.string()
            << " or " << pathModifiedFile.string() << std::endl;
        patchFile.close();
        return;
    }

    // Track the current byte offset
    std::size_t offset = 0;
    bool differencesFound = false;
    std::size_t bytesWritten = 0;
    std::size_t bytesRemoved = 0;

    while (true) {
        char originalByte, modifiedByte;
        originalFile.get(originalByte);
        modifiedFile.get(modifiedByte);

        if (!originalFile && !modifiedFile) {
            // Both files ended, we're done
            break;
        }

        if (originalByte != modifiedByte) {
            // Difference found, write to patch file
            patchFile << "Offset: " << offset << ", Original Byte: "
                << static_cast<int>(originalByte) << ", Modified Byte: "
                << static_cast<int>(modifiedByte) << std::endl;
            differencesFound = true;

            ++bytesWritten; // Count bytes written
        }
        else {
            ++bytesRemoved; // Count bytes removed
        }

        // Increment offset
        ++offset;
    }

    // Close files
    originalFile.close();
    modifiedFile.close();
    patchFile.close();

    if (!differencesFound) {
        std::cout << "No differences found between the files." << std::endl;
        std::filesystem::remove(pathPatchFile); // Remove empty patch file
    }
    else {
        if (g_bVerbose) {
            std::cout << "Patch file generated: " << pathPatchFile.string() << std::endl;
            std::cout << "Bytes written: " << bytesWritten << std::endl;
            std::cout << "Bytes removed: " << bytesRemoved << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    CommandLineParser parser(argc, argv);

    parser.AddOption("-h", "--help", "Display this help message and exit");
    parser.AddOption("-v", "--verbose", "Enable verbose mode");

    parser.AddArgument("original_file", "The source file");
    parser.AddArgument("modified_file", "The file to compare to");
    parser.AddArgument("output_file", "The file to output the patch file to");

    if (!parser.Parse()) {
        std::cerr << "Argument parse failed." << std::endl;
        parser.PrintHelp();
        return EXIT_FAILURE;
    }

    if (parser.OptionExists("-h")) {
        parser.PrintHelp();
        return EXIT_SUCCESS;
    }


    if (parser.OptionExists("-v")) {
        std::cout << "Verbose mode enabled" << std::endl;
        g_bVerbose = true;
    }

    std::filesystem::path pathOriginalFile = parser.GetArgumentValue("original_file");
    std::filesystem::path pathModifiedFile = parser.GetArgumentValue("modified_file");
    std::filesystem::path pathPatchFile = parser.GetArgumentValue("output_file");

    // Generate patch based on command-line arguments
    GeneratePatchFile(pathOriginalFile, pathModifiedFile, pathPatchFile);

    return EXIT_SUCCESS;
}
