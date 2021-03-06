//  Copyright (c) 2012-2013 Thomas Heller
//  Copyright (c) 2012-2013 Andreas Schaefer
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "simulation.hpp"
#include "parameters.hpp"

#include <hpx/hpx_fwd.hpp>
#include <hpx/hpx_init.hpp>

int hpx_main(boost::program_options::variables_map& vm)
{
    vandouken::runSimulation(vm);
    return hpx::finalize();
}

int main(int argc, char **argv)
{
    boost::program_options::options_description
        commandLineParameters("Usage: " HPX_APPLICATION_STRING " [options]");

    vandouken::setupParameters(commandLineParameters, "server");

    return hpx::init(commandLineParameters, argc, argv);
}
