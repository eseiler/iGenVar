#include <seqan3/argument_parser/all.hpp>   // includes all necessary headers

#include <seqan3/range/views/all.hpp>
#include <seqan3/range/views/get.hpp>

#include "detect_breakends/junction_detection.hpp"

// Specialise a mapping from an identifying string to the respective value of your type clustering_methods. With the
// help of this function, you're able to call ./detect_breackends with -c 0 and -c simple_clustering and get the same
// result.
auto enumeration_names(clustering_methods)
{
    return std::unordered_map<std::string_view,
                              clustering_methods>{{"0", clustering_methods::simple_clustering},
                                                  {"simple_clustering",
                                                   clustering_methods::simple_clustering},
                                                  {"1", clustering_methods::hierarchical_clustering},
                                                  {"hierarchical_clustering",
                                                   clustering_methods::hierarchical_clustering},
                                                  {"2", clustering_methods::self_balancing_binary_tree},
                                                  {"self_balancing_binary_tree",
                                                   clustering_methods::self_balancing_binary_tree},
                                                  {"3", clustering_methods::candidate_selection_based_on_voting},
                                                  {"candidate_selection_based_on_voting",
                                                   clustering_methods::candidate_selection_based_on_voting}};
}

struct cmd_arguments
{
    std::filesystem::path alignment_file_path{};
    std::filesystem::path insertion_file_path{};
    std::vector<uint8_t> methods{1, 2, 3, 4};                   // default is using all methods
    clustering_methods clustering_method{simple_clustering};    // default is the simple clustering method
};

void initialize_argument_parser(seqan3::argument_parser & parser, cmd_arguments & args)
{
    parser.info.author = "David Heller & Lydia Buntrock";
    parser.info.app_name = "iGenVar";
    parser.info.man_page_title = "Short and long Read SV Caller";
    parser.info.short_description = "Detect junctions in a read alignment file";
    parser.info.version = "0.0.1";
    parser.info.date = "19-01-2021";    // last update
    parser.info.email = "lydia.buntrock@fu-berlin.de";
    parser.info.long_copyright = "long_copyright";
    parser.info.short_copyright = "short_copyright";
    parser.info.url = "https://github.com/seqan/iGenVar/";

    // Validatiors:
    // seqan3::value_list_validator method_validator{"1", "cigar_string",
    //                                               "2", "split_read",
    //                                               "3", "read_pairs",
    //                                               "4", "read_depth"};
    seqan3::arithmetic_range_validator method_validator{1, 4};
    seqan3::value_list_validator clustering_method_validator {(seqan3::enumeration_names<clustering_methods> | seqan3::views::get<1>)};

    // Options - Input / Output:
    parser.add_positional_option(args.alignment_file_path, "Input read alignments in SAM or BAM format.",
                                 seqan3::input_file_validator{{"sam", "bam"}} );
    parser.add_positional_option(args.insertion_file_path, "Output file for insertion alleles",
                                 seqan3::output_file_validator{{"fa", "fasta"}} );

    // Options - Methods:
    parser.add_option(args.methods, 'm', "method", "Choose the method to be used.",
                      seqan3::option_spec::ADVANCED, method_validator);
    parser.add_option(args.clustering_method, 'c', "clustering_method", "Choose the clustering method to be used.",
                      seqan3::option_spec::ADVANCED, clustering_method_validator);
}

int main(int argc, char ** argv)
{
    seqan3::argument_parser myparser{"detectJunctions", argc, argv};    // initialise myparser
    cmd_arguments args{};
    initialize_argument_parser(myparser, args);

    // Parse the given arguments and catch possible errors.
    try
    {
        myparser.parse();                                               // trigger command line parsing
    }
    catch (seqan3::argument_parser_error const & ext)                   // catch user errors
    {
        seqan3::debug_stream << "[Error] " << ext.what() << '\n';       // customise your error message
        return -1;
    }

    detect_junctions_in_alignment_file(args.alignment_file_path,
                                       args.insertion_file_path,
                                       args.methods,
                                       args.clustering_method);

    return 0;
}
