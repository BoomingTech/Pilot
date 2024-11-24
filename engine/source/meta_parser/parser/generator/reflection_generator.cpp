#include "common/precompiled.h"

#include "generator/reflection_generator.h"

#include "language_types/class.h"
#include "template_manager/template_manager.h"

#include <map>
#include <set>

namespace Generator
{
    ReflectionGenerator::ReflectionGenerator(std::string                             source_directory,
                                             std::function<std::string(std::string)> get_include_function) :
        GeneratorInterface(source_directory + "/_generated/reflection", source_directory, get_include_function)
    {
        prepareStatus(m_out_path);
    }
    void ReflectionGenerator::prepareStatus(std::string path)
    {
        GeneratorInterface::prepareStatus(path);
        TemplateManager::getInstance()->loadTemplates(m_root_path, "commonReflectionFile");
        TemplateManager::getInstance()->loadTemplates(m_root_path, "allReflectionFile");
        return;
    }

    std::string ReflectionGenerator::processFileName(std::string path)
    {
        auto relativeDir = fs::path(path).filename().replace_extension("reflection.gen.h").string();
        return m_out_path + "/" + relativeDir;
    }

    int ReflectionGenerator::generate(std::string path, SchemaMoudle schema)
    {
        static const std::string vector_prefix = "std::vector<";

        std::string    file_path = processFileName(path);

        Mustache::data mustache_data;
        Mustache::data include_headfiles(Mustache::data::type::list);
        Mustache::data class_defines(Mustache::data::type::list);

        include_headfiles.push_back(
            Mustache::data("headfile_name", Utils::makeRelativePath(m_root_path, path).string()));

        std::map<std::string, bool> class_names;
        // class defs
        for (auto class_temp : schema.classes)
        {
            if (!class_temp->shouldCompile())
                continue;

            class_names.insert_or_assign(class_temp->getClassName(), false);
            class_names[class_temp->getClassName()] = true;

            std::vector<std::string>                                   field_names;
            std::map<std::string, std::pair<std::string, std::string>> vector_map;

            Mustache::data class_def;
            Mustache::data vector_defines(Mustache::data::type::list);

            genClassRenderData(class_temp, class_def);
            for (auto field : class_temp->m_fields)
            {
                if (!field->shouldCompile())
                    continue;
                field_names.emplace_back(field->m_name);
                bool is_array = field->m_type.find(vector_prefix) == 0;
                if (is_array)
                {
                    std::string array_useful_name = field->m_type;

                    Utils::formatQualifiedName(array_useful_name);

                    std::string item_type = field->m_type;

                    item_type = Utils::getNameWithoutContainer(item_type);

                    vector_map[field->m_type] = std::make_pair(array_useful_name, item_type);
                }
            }

            if (vector_map.size() > 0)
            {
                if (nullptr == class_def.get("vector_exist"))
                {
                    class_def.set("vector_exist", true);
                }
                for (auto vector_item : vector_map)
                {
                    std::string    array_useful_name = vector_item.second.first;
                    std::string    item_type         = vector_item.second.second;
                    Mustache::data vector_define;
                    Utils::replace(array_useful_name, ',', '_');
                    vector_define.set("vector_useful_name", array_useful_name);
                    vector_define.set("vector_type_name", vector_item.first);
                    vector_define.set("vector_element_type_name", item_type);
                    vector_defines.push_back(vector_define);
                }
            }
            class_def.set("vector_defines", vector_defines);
            class_defines.push_back(class_def);
        }

        mustache_data.set("class_defines", class_defines);
        mustache_data.set("include_headfiles", include_headfiles);

        std::string tmp = Utils::convertNameToUpperCamelCase(fs::path(path).stem().string(), "_");
        mustache_data.set("sourefile_name_upper_camel_case", tmp);

        std::string render_string =
            TemplateManager::getInstance()->renderByTemplate("commonReflectionFile", mustache_data);
        Utils::saveFile(render_string, file_path);

        m_sourcefile_list.emplace_back(tmp);

        m_head_file_list.emplace_back(Utils::makeRelativePath(m_root_path, file_path).string());
        return 0;
    }
    void ReflectionGenerator::finish()
    {
        Mustache::data mustache_data;
        Mustache::data include_headfiles = Mustache::data::type::list;
        Mustache::data sourefile_names    = Mustache::data::type::list;

        for (auto& head_file : m_head_file_list)
        {
            include_headfiles.push_back(Mustache::data("headfile_name", head_file));
        }
        for (auto& sourefile_name_upper_camel_case : m_sourcefile_list)
        {
            sourefile_names.push_back(Mustache::data("sourefile_name_upper_camel_case", sourefile_name_upper_camel_case));
        }
        mustache_data.set("include_headfiles", include_headfiles);
        mustache_data.set("sourefile_names", sourefile_names);
        std::string render_string =
            TemplateManager::getInstance()->renderByTemplate("allReflectionFile", mustache_data);
        Utils::saveFile(render_string, m_out_path + "/all_reflection.h");
    }

    ReflectionGenerator::~ReflectionGenerator() {}
} // namespace Generator