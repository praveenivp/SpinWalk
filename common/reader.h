
/* --------------------------------------------------------------------------
 * Project: Microvascular
 * File: miscellaneous.h
 *
 * Author   : Ali Aghaeifar <ali.aghaeifar@tuebingen.mpg.de>
 * Date     : 10.02.2023
 * Descrip  : simulating BOLD in microvascular network
 * -------------------------------------------------------------------------- */

#ifndef __CONFIG_READER_H__
#define __CONFIG_READER_H__

#include <map>
#include "ini.h"
#include "miscellaneous.h"

namespace reader
{

bool read_config(std::string  config_file, simulation_parameters& param, std::vector<float>& sample_length_scales, std::map<std::string, std::vector<std::string> >& filenames)
{
    if (std::filesystem::exists(config_file) == false)
    {
        std::cout << "File does not exist: " << config_file << std::endl;
        return false;
    }
    // first, create a file instance
    mINI::INIFile file(config_file);
    // next, create a structure that will hold data
    mINI::INIStructure ini;
    if (file.read(ini) == false)
    {
        std::cout << "Problem reading config file: " << config_file << std::endl;
        return false;
    }

    // reading section FILES
    if(ini.has("files"))
    {
        if (ini.get("files").has("FIELD_MAP[0]"))
        {
            filenames.at("fieldmap").clear();
            for (int i = 0; ini.get("files").has("FIELD_MAP[" + std::to_string(i) + "]"); i++)
            {
                filenames.at("fieldmap")
                    .push_back(ini.get("files").get("FIELD_MAP["
                        + std::to_string(i) + "]"));
                if (std::filesystem::exists(filenames.at("fieldmap").back())
                    == false)
                {
                    std::cout << "File does not exist: "
                        << filenames.at("fieldmap").back() << std::endl;
                    return false;
                }
            }
        }
        param.n_fieldmaps = filenames.at("fieldmap").size();

        if (ini.get("files").has("m0"))
        {
            filenames.at("m0").clear();            
            if (std::filesystem::exists(ini.get("files").get("m0")) == false)
                std::cout << "File does not exist: " << ini.get("files").get("m0") << std::endl;
            else
                filenames.at("m0").push_back(ini.get("files").get("m0"));
        }

        if (ini.get("files").has("xyz0"))
        {
            filenames.at("xyz0").clear();            
            if (std::filesystem::exists(ini.get("files").get("xyz0")) == false)
                std::cout << "File does not exist: " << ini.get("files").get("xyz0") << std::endl;
            else
                filenames.at("xyz0").push_back(ini.get("files").get("xyz0"));
        }

        if (ini.get("files").has("OUTPUTS"))
        {
            filenames.at("output").clear();
            filenames.at("output").push_back(ini.get("files").get("OUTPUTS"));
        }
    }

    // reading section SCAN_PARAMETERS
    if(ini.has("SCAN_PARAMETERS"))
    {
        if(ini.get("SCAN_PARAMETERS").has("TR"))
            param.TR = std::stof(ini.get("SCAN_PARAMETERS").get("TR"));    
        if(ini.get("SCAN_PARAMETERS").has("DWELL_TIME"))
            param.dt = std::stof(ini.get("SCAN_PARAMETERS").get("DWELL_TIME"));
        if(ini.get("SCAN_PARAMETERS").has("DUMMY_SCAN"))
            param.n_dummy_scan  = std::stoi(ini.get("SCAN_PARAMETERS").get("DUMMY_SCAN"));
        if(ini.get("SCAN_PARAMETERS").has("FA"))
            param.FA = std::stof(ini.get("SCAN_PARAMETERS").get("FA")) * M_PI / 180.; // convert to radian
        
        param.TE = param.TR / 2.; // std::stof(ini.get("SCAN_PARAMETERS").get("TE"));
    }

    // reading section SIMULATION_PARAMETERS
    if(ini.has("SIMULATION_PARAMETERS"))
    {
        if(ini.get("SIMULATION_PARAMETERS").has("B0"))
            param.B0 = std::stof(ini.get("SIMULATION_PARAMETERS").get("B0"));
        if(ini.get("SIMULATION_PARAMETERS").has("SEED"))
            param.seed = std::stoi(ini.get("SIMULATION_PARAMETERS").get("SEED"));
        if(ini.get("SIMULATION_PARAMETERS").has("NUMBER_OF_SPINS"))
            param.n_spins = std::stof(ini.get("SIMULATION_PARAMETERS").get("NUMBER_OF_SPINS"));
        if(ini.get("SIMULATION_PARAMETERS").has("DIFFUSION_CONSTANT"))
            param.diffusion_const = std::stof(ini.get("SIMULATION_PARAMETERS").get("DIFFUSION_CONSTANT"));
        if(ini.get("SIMULATION_PARAMETERS").has("ENABLE_180_REFOCUSING"))
            param.enRefocusing180 = ini.get("SIMULATION_PARAMETERS").get("ENABLE_180_REFOCUSING").compare("0") != 0;
        if(ini.get("SIMULATION_PARAMETERS").has("SAMPLE_LENGTH_SCALES[0]"))
        {
            sample_length_scales.clear();
             for (int i = 0; ini.get("SIMULATION_PARAMETERS").has("SAMPLE_LENGTH_SCALES[" + std::to_string(i) + "]"); i++)
                sample_length_scales.push_back(std::stof(ini.get("SIMULATION_PARAMETERS").get("SAMPLE_LENGTH_SCALES[" + std::to_string(i) + "]")));
            param.n_sample_length_scales = sample_length_scales.size();
        }
    }

    // reading section TISSUE_PARAMETERS
    if(ini.has("TISSUE_PARAMETERS"))
    {
        if(ini.get("TISSUE_PARAMETERS").has("T1"))
            param.T1 = std::stof(ini.get("TISSUE_PARAMETERS").get("T1"));
        if(ini.get("TISSUE_PARAMETERS").has("T2"))
            param.T2 = std::stof(ini.get("TISSUE_PARAMETERS").get("T2"));
    }

    // reading section DEBUG 
    if(ini.has("DEBUG"))
    {
        if(ini.get("DEBUG").has("DUMP_INFO"))
            param.enDebug = ini.get("DEBUG").get("DUMP_INFO").compare("0") != 0;
        if(ini.get("DEBUG").has("SIMULATE_STEADYSTATE"))
            param.enSteadyStateSimulation  = ini.get("DEBUG").get("SIMULATE_STEADYSTATE").compare("0") != 0;
    }   

    param.prepare(); 
    return true;
}


bool read_fieldmap(std::string fieldmap_file, std::vector<float> &fieldmap, std::vector<char> &mask, simulation_parameters& param)
{
    input_header hdr_in;
    std::cout << "Loading fieldmap: " << fieldmap_file << std::endl;
    std::ifstream in_field(fieldmap_file, std::ios::in | std::ios::binary);
    if (!in_field.is_open()) 
    {
        std::cout << "Error opening file " << fieldmap_file << std::endl;
        return false;
    }

    in_field.read((char*)&hdr_in, sizeof(input_header));
    std::copy(hdr_in.fieldmap_size, hdr_in.fieldmap_size + 3, param.fieldmap_size);
    std::copy(hdr_in.sample_length, hdr_in.sample_length + 3, param.sample_length);
    param.matrix_length = param.fieldmap_size[0] * param.fieldmap_size[1] * param.fieldmap_size[2];
    if (fieldmap.size() != param.matrix_length)
    {
        std::cout << "Fieldmap size changed. Re-allocating memory..." << std::endl;
        std::cout << "Old size: " << fieldmap.size() << std::endl;
        std::cout << "New size: " << param.matrix_length << std::endl;
        std::cout << "New length (um): " << param.sample_length[0] * 1e6 << " " << param.sample_length[1] * 1e6 << " " << param.sample_length[2] * 1e6 << std::endl;
        fieldmap.resize(param.matrix_length);
        mask.resize(param.matrix_length);
    }

    in_field.read((char*)fieldmap.data(), sizeof(float) * param.matrix_length);
    in_field.read((char*)mask.data(), sizeof(bool) * param.matrix_length);
    in_field.close();
    return true;
}

bool read_m0()
{
    return true;
}

}
#endif  // __CONFIG_READER_H__