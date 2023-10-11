/*
    Copyright (C) 2023 Bernd Herzog

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

MEMORY
{
    /* external apps: regions can't overlap so addresses are corrected after build */
    ram_external_app_pacman     (rwx) : org = 0xEEE90000, len = 32k
    ram_external_app_afsk_rx    (rwx) : org = 0xEEEA0000, len = 32k
    ram_external_app_calculator (rwx) : org = 0xEEEB0000, len = 32k
    ram_external_app_font_viewer(rwx) : org = 0xEEEC0000, len = 32k
}

SECTIONS
{
    .external_app_pacman : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_pacman.application_information));
        *(*ui*external_app*pacman*);
    } > ram_external_app_pacman

    .external_app_afsk_rx : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_afsk_rx.application_information));
        *(*ui*external_app*afsk_rx*);
    } > ram_external_app_afsk_rx

    .external_app_calculator : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_calculator.application_information));
        *(*ui*external_app*calculator*);
    } > ram_external_app_calculator

    .external_app_font_viewer : ALIGN(4) SUBALIGN(4)
    {
        KEEP(*(.external_app.app_font_viewer.application_information));
        *(*ui*external_app*font_viewer*);
    } > ram_external_app_font_viewer
}
