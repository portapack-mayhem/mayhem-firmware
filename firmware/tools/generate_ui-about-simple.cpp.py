#!/usr/bin/env python3

# Copysomething (c) 2024 LupusE with the license, needed by the PortaPack project
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

import argparse
import json
import os
import re
import requests
import sys

cppheader = """#include "ui_about_simple.hpp"
#include <string_view>

#define ROLL_SPEED_FRAME_PER_LINE 60
// cuz frame rate of pp screen is probably 60, scroll per sec

namespace ui {

// Information: a line starting with a '#' will be yellow coloured
constexpr std::"""

cppfooter = """
AboutView::AboutView(NavigationView& nav) {
    add_children({&menu_view,
                  &button_ok});

    button_ok.on_select = [&nav](Button&) {
        nav.pop();
    };

    menu_view.on_left = [this]() {
        button_ok.focus();
    };

    menu_view.on_right = [this]() {
        button_ok.focus();
    };

    for (auto& authors_line : mayhem_information_list) {
        // if it's starting with #, it's a title and we have to substract the '#' and paint yellow
        if (authors_line.size() > 0) {
            if (authors_line[0] == '#') {
                menu_view.add_item(
                    {(std::string)authors_line.substr(1, authors_line.size() - 1),
                     ui::Theme::getInstance()->fg_yellow->foreground,
                     nullptr,
                     nullptr});
            } else {
                menu_view.add_item(
                    {(std::string)authors_line,
                     Theme::getInstance()->bg_darkest->foreground,
                     nullptr,
                     nullptr});
            }
        }
    }
}

void AboutView::on_frame_sync() {
    if (interacted) return;

    if (frame_sync_count++ % ROLL_SPEED_FRAME_PER_LINE == 0) {
        const auto current = menu_view.highlighted_index();
        const auto count = menu_view.item_count();

        if (current < count - 1) {
            menu_view.set_highlighted(current + 1);
        } else {
            menu_view.set_highlighted(0);
        }
    }
}

void AboutView::focus() {
    button_ok.focus();
    menu_view.set_highlighted(3);  // contributors block starting line
}

bool AboutView::on_touch(const TouchEvent) {
    interacted = true;
    return false;
}

bool AboutView::on_key(const KeyEvent) {
    interacted = true;
    return false;
}

bool AboutView::on_encoder(const EncoderEvent) {
    interacted = true;
    menu_view.focus();
    return false;
}

} /* namespace ui */
"""

def get_contributors(url):
    contributors_list = []  ## Raw list of contributor names
    logins = json.loads(requests.get(url).text)
    try:
        print(f"Could not reach URL - Error: {logins['status']} Messsage: {logins['message']}")
        ## Status f.ex. 404 / message f.ex. "Not Found" or "API rate limit exceeded for...".
        sys.exit()
    except:
        pass
    
    for gh_user in logins:
        clean_ghuser = re.sub('[^a-zA-Z0-9-]', '.', gh_user['login']) ## replace non printable character with .
        contributors_list.append(clean_ghuser)
    
    contributor_id = 0
    contrib_format = [] ## Formated list of contributors, max 30 char per line
    for contributor in contributors_list:
        contributor_id +=1
        contrib_format.append(contributor)
        line_len = len(",".join(contrib_format).split("\n")[-1])
        if contributor_id < len(contributors_list) and line_len+len(contributors_list[contributor_id]) >=30:
            contrib_format.append("\n")

    return(",".join(contrib_format).replace("\n,","\n")) # replace to erase first , in followup lines

def generate_content(projects):
    project_contrib = []
    project_contrib.append("string_view mayhem_information_list[] = {\n")
    project_contrib.append("    \"#****** Mayhem Community ******\",\n")
    project_contrib.append("    \" \",\n")
    project_contrib.append("    \"  https://discord.mayhem.app\",\n")
    project_contrib.append("    \" \",\n")
    project_contrib.append("    \"#**** List of contributors ****\",\n")
    for project in projects:
        project_contrib.append("    \" \",\n")
        project_contrib.append(f"    \"#{project[0]}:\",\n")

        url = f"https://api.github.com/repos/{project[1]}/{project[2]}/contributors?per_page={project[3]}"
        contrib_mayhem = get_contributors(url).split("\n")
        for line in contrib_mayhem:
            project_contrib.append(f"    \"{line}\",\n")

    project_contrib.append("    \" \"};\n")
    return("".join(project_contrib))


def pp_create_ui_about_simple_cpp(cpp_file, cppheader, cppcontent, cppfooter):
    uiaboutsimplecpp_file = []

    uiaboutsimplecpp_file.append(cppheader)
    uiaboutsimplecpp_file.append(cppcontent)
    uiaboutsimplecpp_file.append(cppfooter)

    with open(cpp_file, "w", encoding="utf-8") as file:
        file.writelines(uiaboutsimplecpp_file)
    
    print("Find your ui_simple_about.cpp at", cpp_file)

def pp_change_ui_about_simple_cpp(cpp_file, cppcontent):
    content = []
    content_pattern = re.compile(r"string_view mayhem_information_list\[\] = {\n(?:\s+(?:.*,\n)+\s+.*};\n)", re.MULTILINE)

    # Read original file
    with open(cpp_file, 'r') as file:
        filedata = file.read()

    # Replace regex content with generated list
    for match in content_pattern.finditer(filedata):
            content.append(match[0])
 
    filedata = filedata.replace(content[0], cppcontent)

    # Write new file
    with open(cpp_file, 'w') as file:
        file.write(filedata)


projects = []
## Format: Project title, Github name, Github repo, Amount of contributors
projects.append(["Mayhem-Firmware","portapack-mayhem","mayhem-firmware","50"])
projects.append(["Havoc","furrtek","portapack-havoc","50"])
projects.append(["PortaPack","sharebrained","portapack-hackrf","50"])
projects.append(["HackRF","mossmann","hackrf","15"])


## processing
#############

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
                description="This tool updates the ui_about_simple.cpp from PortaPack firmware from Github contributors. If no file is given, the file will be generated.",
                )
    parser.add_argument("--cpp", help="Path of the source ui_about_simple.cpp to update.", default="ui_about_simple.cpp")
    parser.add_argument("--new","-n", help="Generate a new file")
    
    args = parser.parse_args()

    if args.cpp:
        cpp_file = os.path.join(os.getcwd(),args.cpp)
    else:
        cpp_file = os.path.join(os.getcwd(),"ui_about_simple.cpp")

    cppcontent = generate_content(projects)

    if args.new or not os.path.isfile(args.cpp):
        pp_create_ui_about_simple_cpp(cpp_file, cppheader, cppcontent, cppfooter)    
    else:
        pp_change_ui_about_simple_cpp(cpp_file, cppcontent)
