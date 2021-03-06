#include <seqan3/core/debug_stream.hpp>

#include "variant_detection/bam_functions.hpp"  // for hasFlag* functions
#include "structures/aligned_segment.hpp"       // for struct AlignedSegment
#include "structures/junction.hpp"              // for class Junction

using seqan3::operator""_tag;

template <class Container>
void split_string(std::string const & str, Container & cont, char const delim = ' ')
{
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        cont.push_back(token);
    }
}

void retrieve_aligned_segments(std::string const & sa_string, std::vector<AlignedSegment> & aligned_segments)
{
    std::vector<std::string> sa_tags{};
    split_string(sa_string, sa_tags, ';');
    for (std::string sa_tag : sa_tags)
    {
        std::vector<std::string> fields {};
        split_string(sa_tag, fields, ',');
        if (fields.size() == 6)
        {
            std::string ref_name = fields[0];
            int32_t pos = std::stoi(fields[1]);
            strand orientation;
            if (fields[2] == "+")
            {
                orientation = strand::forward;
            }
            else if (fields[2] == "-")
            {
                orientation = strand::reverse;
            }
            else
            {
                continue;
            }
            std::string cigar_field = fields[3];
            std::tuple<std::vector<seqan3::cigar>, int32_t, int32_t> parsed_cigar = parse_cigar(cigar_field);
            std::vector<seqan3::cigar> cigar_vector = std::get<0>(parsed_cigar);
            int32_t mapq = std::stoi(fields[4]);
            aligned_segments.push_back(AlignedSegment{orientation, ref_name, pos, mapq, cigar_vector});
        }
        else
        {
            seqan3::debug_stream << "Your SA tag has a wrong format (wrong amount of parameters): " << sa_tag << '\n';
        }
    }
}

void analyze_aligned_segments(std::vector<AlignedSegment> const & aligned_segments,
                              std::vector<Junction> & junctions,
                              seqan3::dna5_vector const & query_sequence,
                              std::string const & read_name)
{
    for(size_t i = 1; i<aligned_segments.size(); i++)
    {
        AlignedSegment current = aligned_segments[i-1];
        AlignedSegment next = aligned_segments[i];
        int32_t distance_on_read = next.get_query_start() - current.get_query_end();
        // Check that there is neither an overlap nor a larger gap on the read
        // TODO(eldarion): add command-line parameters for changing these currently hard-coded cutoffs
        if (distance_on_read >= 0 && distance_on_read <= 10)
        {
            Breakend mate1{current.ref_name,
                           current.orientation == strand::forward ? current.get_reference_end()
                                                                  : current.get_reference_start(),
                           current.orientation};
            Breakend mate2{next.ref_name,
                           next.orientation == strand::forward ? next.get_reference_start()
                                                               : next.get_reference_end(),
                           next.orientation};
            auto inserted_bases = query_sequence | seqan3::views::slice(current.get_query_end(), next.get_query_start());
            Junction new_junction{mate1, mate2, inserted_bases, read_name};
            seqan3::debug_stream << "BND: " << new_junction << "\n";
            junctions.push_back(std::move(new_junction));
        }
    }
}

void analyze_sa_tag(std::string const & query_name,
                    seqan3::sam_flag const & flag,
                    std::string const & ref_name,
                    int32_t const pos,
                    uint8_t const mapq,
                    std::vector<seqan3::cigar> const & cigar,
                    seqan3::dna5_vector const & seq,
                    std::string const & sa_tag,
                    std::vector<Junction> & junctions)
{

    std::vector<AlignedSegment> aligned_segments{};
    strand strand = (hasFlagReverseComplement(flag) ? strand::reverse : strand::forward);
    aligned_segments.push_back(AlignedSegment{strand, ref_name, pos, mapq, cigar});
    retrieve_aligned_segments(sa_tag, aligned_segments);
    std::sort(aligned_segments.begin(), aligned_segments.end());
    analyze_aligned_segments(aligned_segments, junctions, seq, query_name);
}
