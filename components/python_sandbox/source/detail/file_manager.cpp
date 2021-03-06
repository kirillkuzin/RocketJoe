#include <detail/file_manager.hpp>

namespace components { namespace python_sandbox { namespace detail {

    file_view::file_view(const boost::filesystem::path& path)
        : file__(path.c_str(), boost::interprocess::read_write)
        , region(file__, boost::interprocess::read_write) {
        raw_ = boost::string_view(static_cast<char*>(region.get_address()), region.get_size());
    }

    auto file_view::read() -> file_view::storage_t {
        int key = 0;

        boost::string_view delims = "\n";

        for (auto first = raw_.data(), second = raw_.data(), last = first + raw_.size();
             second != last && first != last; first = second + 1) {
            second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

            if (first != second) {
                file_content.emplace(key, boost::string_view(first, second - first).to_string());
                ++key;
            }
        }

        return file_content;
    }

    auto file_manager::open(const boost::filesystem::path& path) -> file_view* {
        if (boost::filesystem::exists(path)) {
            auto it = files_.find(path.string());
            if (it == files_.end()) {
                auto result = files_.emplace(path.string(), std::make_unique<file_view>(path));
                return result.first->second.get();
            } else {
                return it->second.get();
            }
        } else {
            return nullptr;
        }
    }
}}} // namespace components::python_sandbox::detail