#include "ff_synergy.h"
#include "lama_synergy.h"

#include "../evaluation_context.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/system.h"

using namespace std;

namespace landmarks {
FFSynergyHeuristic::FFSynergyHeuristic(const options::Options &opts)
    : Heuristic(opts),
      master(dynamic_cast<LamaSynergyHeuristic *>(
                 opts.get<Heuristic *>("lama_synergy_heuristic"))) {
    cout << "Initializing LAMA-FF synergy slave" << endl;
    if (!master) {
        cerr << "ff_synergy requires a lama_synergy heuristic" << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
}

EvaluationResult FFSynergyHeuristic::compute_result(
    EvaluationContext &eval_context) {
    /*
       Asking for the master's heuristic value triggers both
       heuristic computations if they have not been computed yet.
       If they have been computed yet, then both heuristic values
       are already cached, and this is just a quick lookup. In
       either case, the result is subsequently available in the
       master object.
    */
    eval_context.get_heuristic_value_or_infinity(master);
    return master->ff_result;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "LAMA-FF synergy slave",
        "See documentation for LAMA-FF synergy master.");
    parser.add_option<Heuristic *>("lama_synergy_heuristic");
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return new FFSynergyHeuristic(opts);
}

static Plugin<Heuristic> _plugin("ff_synergy", _parse);
}