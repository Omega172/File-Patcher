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

// Function to apply a patch file to the original file
void ApplyPatchFile(const std::filesystem::path& pathOriginalFile,
    const std::filesystem::path& pathPatchFile) {
    // Read the patch file to verify integrity and apply changes
    std::ifstream patchFile(pathPatchFile);
    if (!patchFile) {
        std::cerr << "Failed to open patch file for reading: " << pathPatchFile.string() << std::endl;
        return;
    }

    // Read and validate header information
    std::string headerLine;
    std::getline(patchFile, headerLine); // Read "PATCH FILE"
    std::getline(patchFile, headerLine); // Read "Original File Hash: ..."
    std::string originalHash = headerLine.substr(headerLine.find(':') + 2);

    std::string calculatedHash = CalculateFileHash(pathOriginalFile);
    if (originalHash != calculatedHash) {
        std::cerr << "Hash mismatch: The patch file is not compatible with the provided original file." << std::endl;
        return;
    }

    // Open the original file for writing
    std::fstream originalFile(pathOriginalFile, std::ios::in | std::ios::out | std::ios::binary);
    if (!originalFile) {
        std::cerr << "Failed to open original file for writing: " << pathOriginalFile.string() << std::endl;
        return;
    }

    // Skip the header lines in the patch file
    for (int i = 0; i < 1; ++i) {
        std::getline(patchFile, headerLine);
    }

    // Apply patch changes to the original file
    std::size_t bytesApplied = 0;
    std::string patchEntry;
    while (std::getline(patchFile, patchEntry)) {
        // Parse patch entry: Offset, Original Byte, Modified Byte
        std::istringstream iss(patchEntry);
        std::string offsetStr, originalByteStr, modifiedByteStr;
        std::getline(iss, offsetStr, ':'); // Read "Offset"
        std::getline(iss, offsetStr, ','); // Read the offset value
        std::getline(iss, originalByteStr, ':'); // Read "Original Byte"
        std::getline(iss, originalByteStr, ','); // Read the original byte value
        std::getline(iss, modifiedByteStr, ':'); // Read "Modified Byte"
        std::getline(iss, modifiedByteStr); // Read the modified byte value

        // Convert strings to integers
        int offset = std::stoi(offsetStr);
        char modifiedByte = static_cast<char>(std::stoi(modifiedByteStr));

        // Seek to the offset in the original file
        originalFile.seekp(offset);

        // Write the modified byte to the original file
        originalFile.put(modifiedByte);

        ++bytesApplied; // Count bytes applied
    }

    // Close files
    originalFile.close();
    patchFile.close();

    if (g_bVerbose) {
        std::cout << "Patch applied successfully to: " << pathOriginalFile.string() << std::endl;
        std::cout << "Bytes applied: " << bytesApplied << std::endl;
    }
}


int main(int argc, char* argv[]) {
    CommandLineParser parser(argc, argv);

    parser.AddOption("-h", "--help", "Display this help message and exit");
    parser.AddOption("-v", "--verbose", "Enable verbose mode");

    parser.AddArgument("target_file", "The to be patched");
    parser.AddArgument("patch_file", "The patch file containing the bytes to be patched");

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

    std::filesystem::path pathTargetFile = parser.GetArgumentValue("target_file");
    std::filesystem::path pathPatchFile = parser.GetArgumentValue("patch_file");

    // Apply patch based on command-line arguments
    ApplyPatchFile(pathTargetFile, pathPatchFile);

    return EXIT_SUCCESS;
}
