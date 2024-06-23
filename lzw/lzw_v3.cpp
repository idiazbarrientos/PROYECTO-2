///
/// @file
/// @author Julius Pettersson
/// @copyright MIT/Expat License.
/// @brief LZW file compressor
/// @version 3
///
/// This is the C++11 implementation of a Lempel-Ziv-Welch single-file command-line compressor.
/// It uses the simpler fixed-width code compression method.
/// It was written with Doxygen comments.
///
/// @see http://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch
/// @see http://marknelson.us/2011/11/08/lzw-revisited/
/// @see http://www.cs.duke.edu/csed/curious/compression/lzw.html
/// @see http://warp.povusers.org/EfficientLZW/index.html
/// @see http://en.cppreference.com/
/// @see http://www.doxygen.org/
///

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <chrono>

/// Type used to store and retrieve codes.
using CodeType = std::uint16_t;

namespace globals {

/// Dictionary Maximum Size (when reached, the dictionary will be reset)
const CodeType dms {std::numeric_limits<CodeType>::max()};

} // namespace globals

///
/// @brief Compresses the contents of `is` and writes the result to `os`.
/// @param [in] is      input stream
/// @param [out] os     output stream
///
void compress(std::istream &is, std::ostream &os)
{
    std::map<std::pair<CodeType, char>, CodeType> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
        {
            // to prevent Undefined Behavior, resulting from reading and modifying
            // the dictionary object at the same time
            const CodeType dictionary_size = dictionary.size();

            dictionary[{globals::dms, static_cast<char> (c)}] = dictionary_size;
        }
    };

    reset_dictionary();

    CodeType i {globals::dms}; // Index
    char c;

    while (is.get(c))
    {
        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms)
            reset_dictionary();

        if (dictionary.count({i, c}) == 0)
        {
            // to prevent Undefined Behavior, resulting from reading and modifying
            // the dictionary object at the same time
            const CodeType dictionary_size = dictionary.size();

            dictionary[{i, c}] = dictionary_size;
            os.write(reinterpret_cast<const char *> (&i), sizeof (CodeType));
            i = dictionary.at({globals::dms, c});
        }
        else
            i = dictionary.at({i, c});
    }

    if (i != globals::dms)
        os.write(reinterpret_cast<const char *> (&i), sizeof (CodeType));
}

///
/// @brief Decompresses the contents of `is` and writes the result to `os`.
/// @param [in] is      input stream
/// @param [out] os     output stream
///
void decompress(std::istream &is, std::ostream &os)
{
    std::vector<std::pair<CodeType, char>> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary] {
        dictionary.clear();
        dictionary.reserve(globals::dms);

        const long int minc = std::numeric_limits<char>::min();
        const long int maxc = std::numeric_limits<char>::max();

        for (long int c = minc; c <= maxc; ++c)
            dictionary.push_back({globals::dms, static_cast<char> (c)});
    };

    const auto rebuild_string = [&dictionary](CodeType k) -> std::vector<char> {
        std::vector<char> s; // String

        while (k != globals::dms)
        {
            s.push_back(dictionary.at(k).second);
            k = dictionary.at(k).first;
        }

        std::reverse(s.begin(), s.end());
        return s;
    };

    reset_dictionary();

    CodeType i {globals::dms}; // Index
    CodeType k; // Key

    while (is.read(reinterpret_cast<char *> (&k), sizeof (CodeType)))
    {
        // dictionary's maximum size was reached
        if (dictionary.size() == globals::dms)
            reset_dictionary();

        if (k > dictionary.size())
            throw std::runtime_error("invalid compressed code");

        std::vector<char> s; // String

        if (k == dictionary.size())
        {
            dictionary.push_back({i, rebuild_string(i).front()});
            s = rebuild_string(k);
        }
        else
        {
            s = rebuild_string(k);

            if (i != globals::dms)
                dictionary.push_back({i, s.front()});
        }

        os.write(&s.front(), s.size());
        i = k;
    }

    if (!is.eof() || is.gcount() != 0)
        throw std::runtime_error("corrupted compressed file");
}

///
/// @brief Prints usage information and a custom error message.
/// @param s    custom error message to be printed
/// @param su   Show Usage information
///
void print_usage(const std::string &s = "", bool su = true)
{
    if (!s.empty())
        std::cerr << "\nERROR: " << s << '\n';

    if (su)
    {
        std::cerr << "\nUsage:\n";
        std::cerr << "\tprogram -flag input_file output_file\n\n";
        std::cerr << "Where `flag' is either `c' for compressing, or `d' for decompressing, and\n";
        std::cerr << "`input_file' and `output_file' are distinct files.\n\n";
        std::cerr << "Examples:\n";
        std::cerr << "\tlzw_v3.exe -c license.txt license.lzw\n";
        std::cerr << "\tlzw_v3.exe -d license.lzw new_license.txt\n";
    }

    std::cerr << std::endl;
}

///
/// @brief Actual program entry point.
/// @param argc             number of command line arguments
/// @param [in] argv        array of command line arguments
/// @retval EXIT_FAILURE    for failed operation
/// @retval EXIT_SUCCESS    for successful operation
///
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        print_usage("Wrong number of arguments.");
        return EXIT_FAILURE;
    }

    enum class Mode {
        Compress,
        Decompress
    };

    Mode m;

    if (std::string(argv[1]) == "-c")
        m = Mode::Compress;
    else
    if (std::string(argv[1]) == "-d")
        m = Mode::Decompress;
    else
    {
        print_usage(std::string("flag `") + argv[1] + "' is not recognized.");
        return EXIT_FAILURE;
    }

    const std::size_t buffer_size {1024 * 1024};

    // these custom buffers should be larger than the default ones
    const std::unique_ptr<char[]> input_buffer(new char[buffer_size]);
    const std::unique_ptr<char[]> output_buffer(new char[buffer_size]);

    std::ifstream input_file;
    std::ofstream output_file;

    input_file.rdbuf()->pubsetbuf(input_buffer.get(), buffer_size);
    input_file.open(argv[2], std::ios_base::binary);

    if (!input_file.is_open())
    {
        print_usage(std::string("input_file `") + argv[2] + "' could not be opened.");
        return EXIT_FAILURE;
    }

    output_file.rdbuf()->pubsetbuf(output_buffer.get(), buffer_size);
    output_file.open(argv[3], std::ios_base::binary);

    if (!output_file.is_open())
    {
        print_usage(std::string("output_file `") + argv[3] + "' could not be opened.");
        return EXIT_FAILURE;
    }

    try
    {
        input_file.exceptions(std::ios_base::badbit);
        output_file.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        if (m == Mode::Compress){
            double tTime = 0;
            auto start = std::chrono::high_resolution_clock::now();
            compress(input_file, output_file);
            auto finish = std::chrono::high_resolution_clock::now();
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
            std::cout<<time.count();
        }
        else
        if (m == Mode::Decompress){
            auto start = std::chrono::high_resolution_clock::now();
            decompress(input_file, output_file);
            auto finish = std::chrono::high_resolution_clock::now();
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
            std::cout<<time.count();
        }
    }
    catch (const std::ios_base::failure &f)
    {
        print_usage(std::string("File input/output failure: ") + f.what() + '.', false);
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        print_usage(std::string("Caught exception: ") + e.what() + '.', false);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
