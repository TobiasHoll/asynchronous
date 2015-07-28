// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_HTML_FORMATTER_HPP
#define BOOST_ASYNC_HTML_FORMATTER_HPP

#include <sstream>
#include <vector>

#include <boost/core/enable_if.hpp>

#include <boost/asynchronous/scheduler_diagnostics.hpp>
#include <boost/asynchronous/diagnostics/scheduler_interface.hpp>

namespace boost { namespace asynchronous {

namespace html_formatter {

// HTML RGB colors to configure html_formatter
struct color
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    std::string to_html() const
    {
        std::stringstream stream;
        stream << "#" << std::hex << std::setfill('0')
               << std::setw(2) << (int) r
               << std::setw(2) << (int) g
               << std::setw(2) << (int) b;
        return stream.str();
    }
};

// Parameters (colors and others) for the html_formatter
struct parameters
{
    std::string title = "Boost.Asynchronous - Scheduler Diagnostics";

    color font                    {0x00, 0x00, 0x00};
    color background              {0xFF, 0xFF, 0xFF};
    color menu_font               {0xFF, 0xFF, 0xFF};
    color menu_background         {0x60, 0x60, 0x60};
    color menu_hover_background   {0x70, 0x70, 0x70};
    color scheduling_background   {0xC8, 0xC8, 0xFF};
    color execution_background    {0xC8, 0xFF, 0xC8};
    color failure_cell_background {0xFF, 0xC8, 0xC8};
    color interrupted_background  {0xFF, 0xFF, 0xC8};
    color total_background        {0xFF, 0xE7, 0xCC};
    color failure_background      {0xFF, 0xC8, 0xC8};
    color table_border            {0x00, 0x00, 0x00};
    color maximum_font            {0xFF, 0x00, 0x00};
    color histogram_font          {0x00, 0x00, 0x00};
    color histogram_lines         {0x00, 0x00, 0x00};
    color histogram_background    {0xFF, 0xFF, 0xFF};

    bool background_override = true;
    bool include_histograms = true;
    bool enable_checkboxes = true;

    std::size_t histogram_bin_count = 20;

    parameters() {}
};

// HTML template and std::ostringstream container

class document
{
private:
    // Constants

    const std::string unsorted_arrows   = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABEAAAAJCAYAAADU6McMAAAALklEQVQoz2NgQAX/GcgAjDgMYCTHkP8ELCBoyH8iXUpbQBWXUC1MKI4dBmqkEwCADwgFjhiWsAAAAABJRU5ErkJggg==";
    const std::string ascending_arrows  = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABEAAAAJCAYAAADU6McMAAAAI0lEQVQoz2NgQAX/GcgAjDgMYCTHkP8ELCBoyH8iXToK6A0A8+YEA7i5INcAAAAASUVORK5CYII=";
    const std::string descending_arrows = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABEAAAAJCAYAAADU6McMAAAAKUlEQVQoz2NgGAU0AYwMDAz/CcgTZQgDDoMYSXEJAxaDGCnx3n9yNAEAjocEA46ycXIAAAAASUVORK5CYII=";

    // Generate document parts

    void generate_header(parameters const& params) {
        header << "<!DOCTYPE html>"                                                               << std::endl
               << "<html>"                                                                        << std::endl
               << "  <head>"                                                                      << std::endl
               << "    <meta charset=\"utf-8\" />"                                                << std::endl
               << "    <title>" << params.title << "</title>"                                     << std::endl
               << "    <style type=\"text/css\">"                                                 << std::endl
               << "      body {"                                                                  << std::endl
               << "        font-family: sans-serif;"                                              << std::endl
               << "        color: " << params.font.to_html() << ";"                               << std::endl
               << "        background-color: " << params.background.to_html() << ";"              << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .index a {"                                                              << std::endl
               << "        text-decoration: none;"                                                << std::endl
               << "        color: " << params.menu_font.to_html() << ";"                          << std::endl
               << "        font-weight: bold;"                                                    << std::endl
               << "        width: 100%;"                                                          << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .index a:hover {}"                                                       << std::endl
               << "      .index a:link {}"                                                        << std::endl
               << "      .index a:active {}"                                                      << std::endl
               << "      .index a:visited {}"                                                     << std::endl
               <<                                                                                    std::endl
               << "      .index ul {"                                                             << std::endl
               << "        list-style-type: none;"                                                << std::endl
               << "        padding: 0;"                                                           << std::endl
               << "        margin: 10px 0 0 0;"                                                   << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .index li {"                                                             << std::endl
               << "        list-style-type: none;"                                                << std::endl
               << "        padding: 7px 10%;"                                                     << std::endl
               << "        white-space: nowrap;"                                                  << std::endl
               << "        overflow: hidden;"                                                     << std::endl
               << "        text-overflow: ellipsis;"                                              << std::endl
               << "      }"                                                                       << std::endl
               << "      .index li:hover {"                                                       << std::endl
               << "        background-color: " << params.menu_hover_background.to_html() << ";"   << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .index {"                                                                << std::endl
               << "        position: fixed;"                                                      << std::endl
               << "        top: 0;"                                                               << std::endl
               << "        left: 0;"                                                              << std::endl
               << "        z-index: 1;"                                                           << std::endl
               << "        color: " << params.menu_font.to_html() << ";"                          << std::endl
               << "        background-color: " << params.menu_background.to_html() << ";"         << std::endl
               << "        text-transform: uppercase;"                                            << std::endl
               << "        font-size: small;"                                                     << std::endl
               << "        width: 15%;"                                                           << std::endl
               << "        height: 100%;"                                                         << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      h1 {"                                                                    << std::endl
               << "        margin-top: 2.5%;"                                                     << std::endl
               << "        margin-bottom: -20px;"                                                 << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      h2 {"                                                                    << std::endl
               << "        margin-top: 75px;"                                                     << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      h4 {"                                                                    << std::endl
               << "        margin-top: 35px;"                                                     << std::endl
               << "        margin-bottom: 10px;"                                                  << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      table.queues {"                                                          << std::endl
               << "        border: 0;"                                                            << std::endl
               << "        margin: -7px 0 -7px 40px;"                                             << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .detail_view .histogram {"                                               << std::endl
               << "        display: inline-block;"                                                << std::endl
               << "        margin-top: 10px;"                                                     << std::endl
               << "      }"                                                                       << std::endl
               << "      button .histogram {"                                                     << std::endl
               << "        height: 40px;"                                                         << std::endl
               << "      }"                                                                       << std::endl
               << "      .histogram_view .histogram {"                                            << std::endl
               << "        flex: 1;"                                                              << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .detail_view table {"                                                    << std::endl
               << "        width: 100%;"                                                          << std::endl
               << "        margin: 10px 0;"                                                       << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .detail_view h3 {"                                                       << std::endl
               << "        flex: 0;"                                                              << std::endl
               << "        margin-top: 0;"                                                        << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .detail_view b {"                                                        << std::endl
               << "        display: block;"                                                       << std::endl
               << "        margin-top: 10px;"                                                     << std::endl
               << "        flex: 0;"                                                              << std::endl
               << "        text-align: center;"                                                   << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .detail_view {"                                                          << std::endl
               << "        display: none;"                                                        << std::endl
               << "        flex-flow: column;"                                                    << std::endl
               << "        justify-content: space-between;"                                       << std::endl
               << "        align-items: center;"                                                  << std::endl
               << "        height: 90%;"                                                          << std::endl
               << "        color: " << params.font.to_html() << ";"                               << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .columns {"                                                              << std::endl
               << "        display: flex;"                                                        << std::endl
               << "        flex: 0;"                                                              << std::endl
               << "        justify-content: space-between;"                                       << std::endl
               << "        width: 100%;"                                                          << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .histogram_view {"                                                       << std::endl
               << "        margin-top: 30px;"                                                     << std::endl
               << "        flex: 1;"                                                              << std::endl
               << "        display: flex;"                                                        << std::endl
               << "        flex-flow: column;"                                                    << std::endl
               << "        width: 85%;"                                                           << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      .noborder, .noborder table, .noborder td, .noborder th {"                << std::endl
               << "        border: 0;"                                                            << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      table {"                                                                 << std::endl
               << "        border-collapse: collapse;"                                            << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      td, th {"                                                                << std::endl
               << "        border: 1px " << params.table_border.to_html() << " solid;"            << std::endl
               << "        padding: 5px;"                                                         << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      table.sortable th[data-column] {"                                        << std::endl
               << "        background-image: url(" << unsorted_arrows << ");"                     << std::endl
               << "        background-repeat: no-repeat;"                                         << std::endl
               << "        background-position: right center;"                                    << std::endl
               << "        padding-right: 27px;"                                                  << std::endl
               << "      }"                                                                       << std::endl
               << "      table.sortable th[data-column][data-order=\"ascending\"] {"              << std::endl
               << "        background-image: url(" << ascending_arrows << ");"                    << std::endl
               << "      }"                                                                       << std::endl
               << "      table.sortable th[data-column][data-order=\"descending\"] {"             << std::endl
               << "        background-image: url(" << descending_arrows << ");"                   << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      td.value {"                                                              << std::endl
               << "        text-align: right;"                                                    << std::endl
               << "      }"                                                                       << std::endl
               << "      td.maximum {"                                                            << std::endl
               << "        color: " << params.maximum_font.to_html() << ";"                       << std::endl
               << "        border-color: " << params.table_border.to_html() << ";"                << std::endl
               << "      }"                                                                       << std::endl
               << "      td.scheduling, th.scheduling {"                                          << std::endl
               << "        background-color: " << params.scheduling_background.to_html() << ";"   << std::endl
               << "      }"                                                                       << std::endl
               << "      td.execution, th.execution {"                                            << std::endl
               << "        background-color: " << params.execution_background.to_html() << ";"    << std::endl
               << "      }"                                                                       << std::endl
               << "      td.failure_cell, th.failure_cell {"                                      << std::endl
               << "        background-color: " << params.failure_cell_background.to_html() << ";" << std::endl
               << "      }"                                                                       << std::endl
               << "      td.interrupted, th.interrupted {"                                        << std::endl
               << "        background-color: " << params.interrupted_background.to_html() << ";"  << std::endl
               << "      }"                                                                       << std::endl
               << "      td.total, th.total {"                                                    << std::endl
               << "        background-color: " << params.total_background.to_html() << ";"        << std::endl
               << "      }"                                                                       << std::endl
               << "      tr.failure {"                                                            << std::endl
               << "        background-color: " << params.failure_background.to_html() << ";"      << std::endl
               << "      }"                                                                       << std::endl
               << "      tr.interruption {"                                                       << std::endl
               << "        background-color: " << params.interrupted_background.to_html() << ";"  << std::endl
               << "      }"                                                                       << std::endl;
        if (params.background_override) {
            header << "      tr.failure > td.scheduling {"                                            << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.failure > td.execution {"                                             << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.failure > td.failure_cell {"                                          << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.failure > td.interrupted {"                                           << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.failure > td.total {"                                                 << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.interruption > td.scheduling {"                                       << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.interruption > td.execution {"                                        << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.interruption > td.failure_cell {"                                     << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.interruption > td.interrupted {"                                      << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl
                   << "      tr.interruption > td.total {"                                            << std::endl
                   << "        background-color: inherit;"                                            << std::endl
                   << "      }"                                                                       << std::endl;
        }
        if (params.enable_checkboxes) {
            header << "      .checkbox {"                                                             << std::endl
                   << "        margin-bottom: 10px;"                                                  << std::endl
                   << "      }"                                                                       << std::endl
                   << "      .checkbox_label {"                                                       << std::endl
                   << "        margin-right: 10px;"                                                   << std::endl
                   << "      }"                                                                       << std::endl
                   << "      .fail_cb:not(:checked) ~ table td.failure_cell,"                         << std::endl
                   << "      .fail_cb:not(:checked) ~ table th.failure_cell {"                        << std::endl
                   << "        display: none;"                                                        << std::endl
                   << "      }"                                                                       << std::endl
                   << "      .int_cb:not(:checked) ~ table td.interrupted,"                           << std::endl
                   << "      .int_cb:not(:checked) ~ table th.interrupted {"                          << std::endl
                   << "        display: none;"                                                        << std::endl
                   << "      }"                                                                       << std::endl
                   << "      .total_cb:not(:checked) ~ table td.total,"                               << std::endl
                   << "      .total_cb:not(:checked) ~ table th.total {"                              << std::endl
                   << "        display: none;"                                                        << std::endl
                   << "      }"                                                                       << std::endl;
        }
        header <<                                                                                    std::endl
               << "      #content {"                                                              << std::endl
               << "        margin-left: 20%;"                                                     << std::endl
               << "        margin-right: 5%;"                                                     << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      #bottom_spacing {"                                                       << std::endl
               << "        width: 100%;"                                                          << std::endl
               << "        height: 1px;"                                                          << std::endl
               << "        margin: 5% 0 0 0;"                                                     << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               << "      #overlay_wrapper {"                                                      << std::endl
               << "        background-color: rgba(0, 0, 0, 0.5);"                                 << std::endl
               << "        position: fixed;"                                                      << std::endl
               << "        top: 0;"                                                               << std::endl
               << "        left: 0;"                                                              << std::endl
               << "        width: 100%;"                                                          << std::endl
               << "        height: 100%;"                                                         << std::endl
               << "        display: none;"                                                        << std::endl
               << "        z-index: 900;"                                                         << std::endl
               << "      }"                                                                       << std::endl
               << "      #overlay {"                                                              << std::endl
               << "        background-color: " << params.background.to_html() << ";"              << std::endl
               << "        position: fixed;"                                                      << std::endl
               << "        top: 10%;"                                                             << std::endl
               << "        left: 10%;"                                                            << std::endl
               << "        height: 80%;"                                                          << std::endl
               << "        width: 80%;"                                                           << std::endl
               << "        border-radius: 5px;"                                                   << std::endl
               << "        overflow: auto;"                                                       << std::endl
               << "      }"                                                                       << std::endl
               << "      #overlay > * {"                                                          << std::endl
               << "        margin: 2.5%;"                                                         << std::endl
               << "      }"                                                                       << std::endl
               <<                                                                                    std::endl
               // SVG styling
               << "      .histogram_bar:hover {"                                                  << std::endl
               << "        opacity: 0.5;"                                                         << std::endl
               << "      }"                                                                       << std::endl
               << "      .histogram_bar + g text {"                                               << std::endl
               << "        font-size: x-small;"                                                   << std::endl
               << "        fill: none;"                                                           << std::endl
               << "        stroke: none;"                                                         << std::endl
               << "        pointer-events: all;"                                                  << std::endl
               << "      }"                                                                       << std::endl
               << "      .histogram_bar:hover + g text {"                                         << std::endl
               << "        fill: " << params.histogram_font.to_html() << ";"                      << std::endl
               << "      }"                                                                       << std::endl
               << "    </style>"                                                                  << std::endl
               << "    <script type=\"text/javascript\">"                                         << std::endl
               << "      function copyTo(nodes, target) {"                                        << std::endl
               << "        while (target.firstChild) {"                                           << std::endl
               << "          target.removeChild(target.firstChild);"                              << std::endl
               << "        }"                                                                     << std::endl
               << "        for (var index = 0; index < nodes.length; ++index) {"                  << std::endl
               << "          target.appendChild(nodes[index].cloneNode(true));"                   << std::endl
               << "        }"                                                                     << std::endl
               << "      }"                                                                       << std::endl
               << "      function copyToOverlay(node) {"                                          << std::endl
               << "        overlay = document.getElementById(\"overlay\");"                       << std::endl
               << "        overlay.appendChild(node.cloneNode(true));"                            << std::endl
               << "        makeTablesSortable(overlay);"                                          << std::endl
               << "        overlay.firstChild.style.display = 'flex';"                            << std::endl
               << "        wrapper = document.getElementById(\"overlay_wrapper\");"               << std::endl
               << "        wrapper.style.display = 'block';"                                      << std::endl
               << "      }"                                                                       << std::endl
               << "      function cancelOverlay(node) {"                                          << std::endl
               << "        overlay = document.getElementById(\"overlay\");"                       << std::endl
               << "        while (overlay.firstChild) {"                                          << std::endl
               << "          overlay.removeChild(overlay.firstChild);"                            << std::endl
               << "        }"                                                                     << std::endl
               << "        wrapper = document.getElementById(\"overlay_wrapper\");"               << std::endl
               << "        wrapper.style.display = 'none';"                                       << std::endl
               << "      }"                                                                       << std::endl
               << "      function cancelEvent(event) {"                                           << std::endl
               << "        event.stopPropagation();"                                              << std::endl
               << "      }"                                                                       << std::endl
               // Sort 'table'. Assumes that only one <tbody> tag exists per table
               << "      function sort(table, column, order) {"                                   << std::endl
               << "        // 'order' is 1 for normal sort, -1 for reverse"                       << std::endl
               << "        var body = table.tBodies[0];"                                          << std::endl
               << "        // Convert rows (HTMLCollection) to array"                             << std::endl
               << "        var rows = Array.prototype.slice.call(body.rows, 0);"                  << std::endl
               << "        rows.sort(function (a, b) {"                                           << std::endl
               << "          // Compare two rows. -1 sorts a before b, 1 sorts b before a"        << std::endl
               << "          var valueA = parseInt(a.cells[column].getAttribute('data-sort'));"   << std::endl
               << "          var valueB = parseInt(b.cells[column].getAttribute('data-sort'));"   << std::endl
               << "          if (valueA < valueB) return -1 * order;"                             << std::endl
               << "          else if (valueB < valueA) return 1 * order;"                         << std::endl
               << "          else return 0;"                                                      << std::endl
               << "        });"                                                                   << std::endl
               << "        for (var i = 0; i < rows.length; ++i) {"                               << std::endl
               << "          body.appendChild(rows[i]);"                                          << std::endl
               << "        }"                                                                     << std::endl
               << "        // Update header arrows"                                               << std::endl
               << "        var header = table.tHead;"                                             << std::endl
               << "        var row = header.rows.length;"                                         << std::endl
               << "        while (--row >= 0) {"                                                  << std::endl
               << "          var cells = header.rows[row].cells;"                                 << std::endl
               << "          var cell = cells.length;"                                            << std::endl
               << "          while (--cell >= 0) {"                                               << std::endl
               << "            if (cells[cell].hasAttribute('data-column')) {"                    << std::endl
               << "              if (cells[cell].getAttribute('data-column') == column) {"        << std::endl
               << "                if (order === 1) {"                                            << std::endl
               << "                  cells[cell].setAttribute('data-order', 'ascending');"        << std::endl
               << "                } else {"                                                      << std::endl
               << "                  cells[cell].setAttribute('data-order', 'descending');"       << std::endl
               << "                }"                                                             << std::endl
               << "              } else {"                                                        << std::endl
               << "                cells[cell].setAttribute('data-order', 'unsorted');"           << std::endl
               << "              }"                                                               << std::endl
               << "            }"                                                                 << std::endl
               << "          }"                                                                   << std::endl
               << "        }"                                                                     << std::endl
               << "      }"                                                                       << std::endl
               // Make 'table' sortable using header cells with a 'data-column' value
               << "      function makeTableSortable(table) {"                                     << std::endl
               << "        var header = table.tHead;"                                             << std::endl
               << "        // Iterate over all header cells"                                      << std::endl
               << "        var row = header.rows.length;"                                         << std::endl
               << "        while (--row >= 0) {"                                                  << std::endl
               << "          var cells = header.rows[row].cells;"                                 << std::endl
               << "          var cell = cells.length;"                                            << std::endl
               << "          while (--cell >= 0) {"                                               << std::endl
               << "            // Check whether the current cell is marked as a key cell"         << std::endl
               << "            if (cells[cell].hasAttribute('data-column')) {"                    << std::endl
               << "              // This cell can be used to sort the appropriate column"         << std::endl
               << "              // Use a function to allow 'order' to change on each click"      << std::endl
               << "              (function (index, sort_id) {"                                    << std::endl
               << "                var order = 1;"                                                << std::endl
               << "                cells[index].addEventListener('click',"                        << std::endl
               << "                                              function () {"                   << std::endl
               << "                                                sort(table,"                   << std::endl
               << "                                                     sort_id,"                 << std::endl
               << "                                                     (order = -order));"       << std::endl
               << "                                              });"                             << std::endl
               << "              }(cell, cells[cell].getAttribute('data-column')))"               << std::endl
               << "            }"                                                                 << std::endl
               << "          }"                                                                   << std::endl
               << "        }"                                                                     << std::endl
               << "      }"                                                                       << std::endl
               // Grab all tables with class 'sortable' and make them sortable
               << "      function makeTablesSortable(node) {"                                     << std::endl
               << "        // Find all tables"                                                    << std::endl
               << "        var tables = node.getElementsByTagName('table');"                      << std::endl
               << "        var index = tables.length;"                                            << std::endl
               << "        while (--index >= 0) {"                                                << std::endl
               << "          var fromSource = ' ' + tables[index].className + ' ';"               << std::endl
               << "          // Check for class 'sortable'"                                       << std::endl
               << "          // getElementsByClassName + checking the tag is also possible,"      << std::endl
               << "          // but has less browser support."                                    << std::endl
               << "          if (fromSource.indexOf(' sortable ') > -1) {"                        << std::endl
               << "            makeTableSortable(tables[index]);"                                 << std::endl
               << "          }"                                                                   << std::endl
               << "        }"                                                                     << std::endl
               << "      }"                                                                       << std::endl
               << "    </script>"                                                                 << std::endl
               << "  </head>"                                                                     << std::endl
               << "  <body onload=\"makeTablesSortable(document);\">"                             << std::endl
               << "    <div id=\"overlay_wrapper\" onclick=\"cancelOverlay();\">"                 << std::endl
               << "      <div id=\"overlay\" onclick=\"cancelEvent(arguments[0]);\"></div>"       << std::endl
               << "    </div>"                                                                    << std::endl
               << "    <div class=\"index\">"                                                     << std::endl
               << "      <ul>"                                                                    << std::endl;
    }

    void generate_body(parameters const& params)
    {
        body << "      </ul>"                           << std::endl
             << "    </div>"                            << std::endl
             << "    <div id=\"content\">"              << std::endl
             << "      <h1>" << params.title << "</h1>" << std::endl;
    }

    void generate_footer(parameters const& params)
    {
        footer << "    </div>"                            << std::endl
               << "    <div id=\"bottom_spacing\"></div>" << std::endl // <div /> is not parsed correctly
               << "  </body>"                             << std::endl
               << "</html>"                               << std::endl;
    }

public:
    std::ostringstream header;
    std::ostringstream body;
    std::ostringstream footer;

    document(boost::asynchronous::html_formatter::parameters const& params)
    {
        generate_header(params);
        generate_body(params);
        generate_footer(params);
    }

    std::string str() const
    {
        return header.str() + body.str() + footer.str();
    }
};

// Details and helpers
namespace detail {

// Escape a string to embed in HTML documents
std::string escape_html(std::string const& str) {
    std::string buffer;
    buffer.reserve(str.size());
    for (std::string::size_type index = 0; index < str.size(); ++index) {
        switch (str[index]) {
        case '&': buffer.append("&amp;");        break;
        case '<': buffer.append("&lt;");         break;
        case '>': buffer.append("&gt;");         break;
        default:  buffer.append(&str[index], 1); break;
        }
    }
    return buffer;
}

// Convert time (boost::chrono::nanoseconds) to string
std::string format_duration(boost::chrono::nanoseconds const& d)
{
    // Get microsecond ticks
    boost::chrono::microseconds casted = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
    boost::int_least64_t ticks = casted.count();

    // Extract values
    boost::int_least64_t seconds = ticks / 1000000;
    boost::int_least16_t milliseconds = (ticks % 1000000) / 1000;
    boost::int_least16_t microseconds = ticks % 1000;

    // Convert to string and return
    std::stringstream stream;
    stream << std::setfill('0');
    if (seconds > 0) stream << seconds << "." << std::setw(3);
    if (seconds > 0 || milliseconds > 0) stream << milliseconds << "." << std::setw(3);
    stream << microseconds;
    return stream.str();
}

// Histogram data
struct histogram
{
    struct bin
    {
        std::size_t count;
        boost::chrono::nanoseconds total;
    };

    enum class draw_type
    {
        COUNT,
        TOTAL
    };

    boost::chrono::nanoseconds min;
    boost::chrono::nanoseconds max;
    std::vector<bin> bins;

    histogram(boost::chrono::nanoseconds min_, boost::chrono::nanoseconds max_, std::size_t bin_count)
        : min(min_), max(max_), bins(bin_count)
    {
        if (bins.size() == 0) throw std::logic_error("Invalid histogram with 0 bins");
    }

    void add(boost::chrono::nanoseconds const& value)
    {
        if (bins.size() == 0) throw std::logic_error("Invalid histogram with 0 bins");
        if (max < value || min > value) throw std::logic_error("Cannot insert value into histogram: Boundaries exceeded.");
        std::size_t bin = ((double) (value.count() - min.count())) / ((double) (max.count() - min.count() + 1)) * bins.size();
        ++(bins[bin].count);
        bins[bin].total += value;
    }

    void draw(document & doc, draw_type type, color const& fill, parameters const& params) const
    {
        if (bins.size() == 0) throw std::logic_error("Invalid histogram with 0 bins");

        // Coordinates and sizes

        // Use 20 viewbox pixels per bin plus 1/9 in spacing.
        std::size_t viewbox_width = bins.size() * 20 * (10.0 / 9.0);
        std::size_t viewbox_height = viewbox_width * 0.6;

        // Add 50% of the viewbox for the labels
        std::size_t label_width = viewbox_width * 0.5;

        std::size_t x_left = viewbox_width * 0.1;
        std::size_t x_right = viewbox_width * 0.9;
        std::size_t y_top = viewbox_height * 0.1;
        std::size_t y_bottom = viewbox_height * 0.9;

        std::size_t label_left = viewbox_width;
        std::size_t label_right = viewbox_width + label_width - (viewbox_width * 0.1);
        std::size_t label_top = viewbox_height * 0.4;
        std::size_t label_bottom = viewbox_height * 0.6;

        std::size_t label_height = label_bottom - label_top;

        std::size_t label_text_left = label_left + (label_width * 0.1);
        std::size_t label_text_top = label_top + (label_height * 0.1);
        std::size_t label_text_bottom = label_bottom - (label_height * 0.1);

        // Drawing common elements

        // Add main tag, draw background
        doc.body << "                      <svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\""     << std::endl
                 << "                           viewBox=\"0 0 " << (viewbox_width + label_width) << " " << viewbox_height << "\" class=\"histogram\">" << std::endl
                 // Draw background
                 << "                        <rect width=\"100%\" height=\"100%\" style=\"fill: " << params.histogram_background.to_html() << ";\" />" << std::endl;

        // Draw label background
        doc.body << "                        <rect x=\"" << label_left << "\" y=\"" << label_top << "\""                                           << std::endl
                 << "                              width=\"" << (label_right - label_left) << "\" height=\"" << (label_bottom - label_top) << "\"" << std::endl
                 << "                              style=\"fill: " << fill.to_html() << "; opacity: 0.5;\" />"                                     << std::endl;

        // Determine maximum value
        std::size_t maximum=0;

        std::vector<std::size_t> numbers(bins.size());
        std::vector<std::string> strings(bins.size());
        for (std::size_t bin_id = 0; bin_id < bins.size(); ++bin_id) {
            switch (type) {
            case draw_type::COUNT:
                numbers[bin_id] = bins[bin_id].count;
                strings[bin_id] = std::to_string(bins[bin_id].count) + " occurrences";
                break;
            case draw_type::TOTAL:
                numbers[bin_id] = bins[bin_id].total.count();
                strings[bin_id] = format_duration(bins[bin_id].total) + " (s.ms.&micro;s)";
                break;
            }
            if (bin_id == 0 || maximum < numbers[bin_id]) maximum = numbers[bin_id];
        }

        // Scale
        double scale = ((double) (y_bottom - y_top)) / ((double) maximum);

        // Draw bars
        std::size_t bin_width = (x_right - x_left) / (bins.size() + 1);
        for (std::size_t bin_id = 0; bin_id < bins.size(); ++bin_id) {
            std::size_t x_start = x_left + bin_width * bin_id;
            std::size_t x_end = x_start + bin_width;
            std::size_t height = (std::size_t) (numbers[bin_id] * scale);
            std::size_t top = y_bottom - height;

            boost::chrono::nanoseconds left_border = min + ((max - min) * bin_id / bins.size());
            boost::chrono::nanoseconds right_border = min + ((max - min) * (bin_id + 1) / bins.size());

            doc.body << "                        <polygon points=\"" << x_start << ", " << top      << " "
                                                                   << x_end   << ", " << top      << " "
                                                                   << x_end   << ", " << y_bottom << " "
                                                                   << x_start << ", " << y_bottom << "\" style=\"fill: " << fill.to_html() << ";\""              << std::endl
                     << "                                 class=\"histogram_bar\"/>"                                                                             << std::endl
                     << "                        <g>"                                                                                                            << std::endl
                     << "                          <text x=\"" << label_text_left << "\" y=\"" << (label_text_top + 10) << "\">" << strings[bin_id] << "</text>" << std::endl
                     << "                          <text x=\"" << label_text_left << "\" y=\"" << label_text_bottom << "\">"                                     << std::endl
                     << "                            (" << format_duration(left_border) << " to " << format_duration(right_border) << ")"                        << std::endl
                     << "                          </text>"                                                                                                      << std::endl
                     << "                        </g>"                                                                                                           << std::endl;
        }

        // Draw axes (over the polygons)
        doc.body << "                        <g style=\"stroke-width: 1; stroke: " << params.histogram_lines.to_html() << ";\">"                              << std::endl
                 << "                          <line x1=\"" << x_left << "\" y1=\"" << y_top << "\" x2=\"" << x_left << "\" y2=\"" << y_bottom << "\" />"     << std::endl
                 << "                          <line x1=\"" << x_left << "\" y1=\"" << y_bottom << "\" x2=\"" << x_right << "\" y2=\"" << y_bottom << "\" />" << std::endl
                 << "                        </g>"                                                                                                            << std::endl;

        // Close all tags and finish
        doc.body << "                      </svg>" << std::endl;
    }

};

// Details for a full table row: individual job statistics
struct row_detail
{
    boost::asynchronous::html_formatter::parameters params;
    boost::asynchronous::summary_diagnostic_item summary_item;
    std::vector<boost::asynchronous::simple_diagnostic_item> items;

    bool has_detail; // Is there detailed information available to show
    bool has_histograms; // Generate histograms?

    row_detail(parameters const& params_,
               boost::asynchronous::summary_diagnostic_item const& item_,
               std::vector<boost::asynchronous::simple_diagnostic_item> const& items_ = std::vector<boost::asynchronous::simple_diagnostic_item>())
        : params(params_), summary_item(item_), items(items_)
    {
        has_detail = items.size() > 1;
        has_histograms = has_detail && params.include_histograms;
    }

    // Create a detail table listing every individual task executed for this job name
    void generate_table(boost::asynchronous::html_formatter::document & doc) const
    {
        if (!has_detail) return;

        doc.body << "                  <table class=\"sortable\">"                                                            << std::endl
                 << "                    <thead>"                                                                             << std::endl
                 << "                      <tr>"                                                                              << std::endl
                 << "                        <th data-column=\"0\">Job name</th>"                                             << std::endl
                 << "                        <th class=\"scheduling\" data-column=\"1\">Scheduling time (s.ms.&micro;s)</th>" << std::endl
                 << "                        <th class=\"execution\" data-column=\"2\">Execution time (s.ms.&micro;s)</th>"   << std::endl
                 << "                        <th class=\"total\" data-column=\"3\">Total time (s.ms.&micro;s)</th>"           << std::endl
                 << "                      </tr>"                                                                             << std::endl
                 << "                    </thead>"                                                                            << std::endl
                 << "                    <tbody>"                                                                             << std::endl;

        for (std::size_t individual_id = 0; individual_id < items.size(); ++individual_id)
        {
            boost::asynchronous::simple_diagnostic_item const& individual = items[individual_id];

            bool is_scheduling_maximum = (!individual.failed && !individual.interrupted && individual.scheduling == summary_item.scheduling_max);
            bool is_execution_maximum  = (!individual.failed && !individual.interrupted && individual.execution  == summary_item.execution_max);
            bool is_total_maximum      = (!individual.failed && !individual.interrupted && individual.total      == summary_item.total_max);

            std::string row_class;
            if (individual.failed) row_class = " class=\"failure\"";
            else if (individual.interrupted) row_class = " class=\"interruption\"";

            doc.body << "                    <tr" << row_class << ">" << std::endl
                     << "                      <td data-sort=\"" << individual_id << "\">" << boost::asynchronous::html_formatter::detail::escape_html(individual.job_name) << "</td>" << std::endl
                     << "                      <td class=\"value scheduling" << (is_scheduling_maximum ? " maximum" : "") << "\" data-sort=\"" << individual.scheduling.count() << "\">" << boost::asynchronous::html_formatter::detail::format_duration(individual.scheduling) << "</td>" << std::endl
                     << "                      <td class=\"value execution"  << (is_execution_maximum ? " maximum" : "")  << "\" data-sort=\"" << individual.execution.count() << "\">"  << boost::asynchronous::html_formatter::detail::format_duration(individual.execution)  << "</td>" << std::endl
                     << "                      <td class=\"value total"      << (is_total_maximum ? " maximum" : "")      << "\" data-sort=\"" << individual.total.count() << "\">"      << boost::asynchronous::html_formatter::detail::format_duration(individual.total)      << "</td>" << std::endl
                     << "                    </tr>" << std::endl;
        }

        doc.body << "                    </tbody>" << std::endl
                 << "                  </table>"   << std::endl;
    }

    void generate_histograms(document & doc) const
    {
        if (!has_histograms) return;
        histogram scheduling_hist { summary_item.scheduling_min, summary_item.scheduling_max, params.histogram_bin_count };
        histogram execution_hist  { summary_item.execution_min,  summary_item.execution_max,  params.histogram_bin_count };
        histogram total_hist      { summary_item.total_min,      summary_item.total_max,      params.histogram_bin_count };

        for (simple_diagnostic_item const& item : items) {
            scheduling_hist.add(item.scheduling);
            execution_hist.add(item.execution);
            total_hist.add(item.total);
        }

        doc.body << "                  <div class=\"columns\">"                                                                           << std::endl
                 << "                    <button type=\"button\" onclick=\"copyTo(this.children, this.parentNode.nextElementSibling);\">" << std::endl
                 << "                      <b>Scheduling time - Occurrences</b>"                                                          << std::endl;
        scheduling_hist.draw(doc, histogram::draw_type::COUNT, params.scheduling_background, params);
        doc.body << "                    </button>"                                                                                       << std::endl
                 << "                    <button type=\"button\" onclick=\"copyTo(this.children, this.parentNode.nextElementSibling);\">" << std::endl
                 << "                      <b>Scheduling time - Time taken</b>"                                                           << std::endl;
        scheduling_hist.draw(doc, histogram::draw_type::TOTAL, params.scheduling_background, params);
        doc.body << "                    </button>"                                                                                       << std::endl
                 << "                    <button type=\"button\" onclick=\"copyTo(this.children, this.parentNode.nextElementSibling);\">" << std::endl
                 << "                      <b>Execution time - Occurrences</b>"                                                           << std::endl;
        execution_hist.draw(doc, histogram::draw_type::COUNT, params.execution_background, params);
        doc.body << "                    </button>"                                                                                       << std::endl
                 << "                    <button type=\"button\" onclick=\"copyTo(this.children, this.parentNode.nextElementSibling);\">" << std::endl
                 << "                      <b>Execution time - Time taken</b>"                                                            << std::endl;
        execution_hist.draw(doc, histogram::draw_type::TOTAL, params.execution_background, params);
        doc.body << "                    </button>"                                                                                       << std::endl
                 << "                    <button type=\"button\" onclick=\"copyTo(this.children, this.parentNode.nextElementSibling);\">" << std::endl
                 << "                      <b>Total time - Occurrences</b>"                                                               << std::endl;
        total_hist.draw(doc, histogram::draw_type::COUNT, params.total_background, params);
        doc.body << "                    </button>"                                                                                       << std::endl
                 << "                    <button type=\"button\" onclick=\"copyTo(this.children, this.parentNode.nextElementSibling);\">" << std::endl
                 << "                      <b>Total time - Time taken</b>"                                                                << std::endl;
        total_hist.draw(doc, histogram::draw_type::TOTAL, params.total_background, params);
        doc.body << "                    </button>"                                                                                       << std::endl
                 << "                  </div>"                                                                                            << std::endl
                 << "                  <div class=\"histogram_view\"></div>"                                                              << std::endl;
    }
};

constexpr const char* histogram_button = "                <svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewbox=\"0 0 100 100\" width=\"20px\" height=\"20px\">\n"
                                         "                  <line x1=\"10\" y1=\"10\" x2=\"10\" y2=\"90\" style=\"stroke: #000000; stroke-width: 5;\" />\n"
                                         "                  <line x1=\"10\" y1=\"90\" x2=\"90\" y2=\"90\" style=\"stroke: #000000; stroke-width: 5;\" />\n"
                                         "                  <rect x=\"20\" y=\"35\" width=\"15\" height=\"45\" style=\"fill: #000000; stroke: #000000;\" />\n"
                                         "                  <rect x=\"35\" y=\"60\" width=\"15\" height=\"20\" style=\"fill: #000000; stroke: #000000;\" />\n"
                                         "                  <rect x=\"50\" y=\"50\" width=\"15\" height=\"30\" style=\"fill: #000000; stroke: #000000;\" />\n"
                                         "                  <rect x=\"65\" y=\"20\" width=\"15\" height=\"60\" style=\"fill: #000000; stroke: #000000;\" />\n"
                                         "                </svg>\n";

constexpr const char* detail_button = "                <svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewbox=\"0 0 100 100\" width=\"20px\" height=\"20px\">\n"
                                      "                  <rect x=\"10\" y=\"40\" width=\"80\" height=\"20\" style=\"fill: #000000; stroke: #000000;\" />\n"
                                      "                  <rect x=\"40\" y=\"10\" width=\"20\" height=\"80\" style=\"fill: #000000; stroke: #000000;\" />\n"
                                      "                </svg>\n";

// Add a table header for a full table to the document
void begin_table(document & doc, parameters const& params, bool has_fails, bool has_interrupts) {

    // Prepare checkboxes
    std::string check = " checked=\"checked\"";
    std::string check_failures = has_fails ? check : "";
    std::string check_interrupts = has_interrupts ? check : "";
    // The checkbox for total time will always be checked

    // Add checkboxes (if enabled) and table
    doc.body << "      <div>"                                                                                        << std::endl;
    if (params.enable_checkboxes) {
    doc.body << "        <input type=\"checkbox\" class=\"checkbox fail_cb\"" << check_failures << " />"             << std::endl
             << "        <span class=\"checkbox_label\">Failure time</span>"                                         << std::endl
             << "        <input type=\"checkbox\" class=\"checkbox int_cb\"" << check_interrupts << " />"            << std::endl
             << "        <span class=\"checkbox_label\">Interruption time</span>"                                    << std::endl
             << "        <input type=\"checkbox\" class=\"checkbox total_cb\"" << check << " />"                     << std::endl
             << "        <span class=\"checkbox_label\">Total time</span>"                                           << std::endl;
    }
    doc.body << "        <table class=\"sortable\">"                                                                 << std::endl
             << "          <thead>"                                                                                  << std::endl
             << "            <tr>"                                                                                   << std::endl
             << "              <th rowspan=\"2\" data-column=\"0\">Job name</th>"                                    << std::endl
             << "              <th colspan=\"5\" class=\"scheduling\">Scheduling time (s.ms.&micro;s)</th>"          << std::endl
             << "              <th colspan=\"5\" class=\"execution\">Successful execution time (s.ms.&micro;s)</th>" << std::endl
             << "              <th colspan=\"5\" class=\"failure_cell\">Failure time (s.ms.&micro;s)</th>"           << std::endl
             << "              <th colspan=\"5\" class=\"interrupted\">Interruption time (s.ms.&micro;s)</th>"       << std::endl
             << "              <th colspan=\"5\" class=\"total\">Total time (s.ms.&micro;s)</th>"                    << std::endl
             << "              <th rowspan=\"2\" class=\"noborder\"></th>"                                           << std::endl
             << "              <th rowspan=\"2\" class=\"noborder\"></th>"                                           << std::endl
             << "            </tr>"                                                                                  << std::endl
             << "            <tr>"                                                                                   << std::endl
             << "              <th class=\"scheduling\" data-column=\"1\">total</th>"                                << std::endl
             << "              <th class=\"scheduling\" data-column=\"2\">average</th>"                              << std::endl
             << "              <th class=\"scheduling\" data-column=\"3\">max.</th>"                                 << std::endl
             << "              <th class=\"scheduling\" data-column=\"4\">min.</th>"                                 << std::endl
             << "              <th class=\"scheduling\" data-column=\"5\">count</th>"                                << std::endl
             << "              <th class=\"execution\" data-column=\"6\">total</th>"                                 << std::endl
             << "              <th class=\"execution\" data-column=\"7\">average</th>"                               << std::endl
             << "              <th class=\"execution\" data-column=\"8\">max.</th>"                                  << std::endl
             << "              <th class=\"execution\" data-column=\"9\">min.</th>"                                  << std::endl
             << "              <th class=\"execution\" data-column=\"10\">count</th>"                                << std::endl
             << "              <th class=\"failure_cell\" data-column=\"11\">total</th>"                             << std::endl
             << "              <th class=\"failure_cell\" data-column=\"12\">average</th>"                           << std::endl
             << "              <th class=\"failure_cell\" data-column=\"13\">max.</th>"                              << std::endl
             << "              <th class=\"failure_cell\" data-column=\"14\">min.</th>"                              << std::endl
             << "              <th class=\"failure_cell\" data-column=\"15\">count</th>"                             << std::endl
             << "              <th class=\"interrupted\" data-column=\"16\">total</th>"                              << std::endl
             << "              <th class=\"interrupted\" data-column=\"17\">average</th>"                            << std::endl
             << "              <th class=\"interrupted\" data-column=\"18\">max.</th>"                               << std::endl
             << "              <th class=\"interrupted\" data-column=\"19\">min.</th>"                               << std::endl
             << "              <th class=\"interrupted\" data-column=\"20\">count</th>"                              << std::endl
             << "              <th class=\"total\" data-column=\"21\">total</th>"                                    << std::endl
             << "              <th class=\"total\" data-column=\"22\">average</th>"                                  << std::endl
             << "              <th class=\"total\" data-column=\"23\">max.</th>"                                     << std::endl
             << "              <th class=\"total\" data-column=\"24\">min.</th>"                                     << std::endl
             << "              <th class=\"total\" data-column=\"25\">count</th>"                                    << std::endl
             << "            </tr>"                                                                                  << std::endl
             << "          </thead>"                                                                                 << std::endl
             << "          <tbody>"                                                                                  << std::endl;
}

// Add a row to the full table.
void add_row(document & doc, summary_diagnostic_item const& item, summary_diagnostics const& data, row_detail detail, std::size_t id) {

    // Determine maxima
    bool is_max_total_scheduling = (data.scheduling_maxima_set && item.scheduling_total   == data.max_scheduling_total);
    bool is_max_avg_scheduling   = (data.scheduling_maxima_set && item.scheduling_average == data.max_scheduling_average);
    bool is_max_max_scheduling   = (data.scheduling_maxima_set && item.scheduling_max     == data.max_scheduling_max);
    bool is_max_min_scheduling   = (data.scheduling_maxima_set && item.scheduling_min     == data.max_scheduling_min);

    bool is_max_total_execution = (data.execution_maxima_set && item.execution_total   == data.max_execution_total);
    bool is_max_avg_execution   = (data.execution_maxima_set && item.execution_average == data.max_execution_average);
    bool is_max_max_execution   = (data.execution_maxima_set && item.execution_max     == data.max_execution_max);
    bool is_max_min_execution   = (data.execution_maxima_set && item.execution_min     == data.max_execution_min);

    bool is_max_total_failure = (data.failure_maxima_set && item.failure_total   == data.max_failure_total);
    bool is_max_avg_failure   = (data.failure_maxima_set && item.failure_average == data.max_failure_average);
    bool is_max_max_failure   = (data.failure_maxima_set && item.failure_max     == data.max_failure_max);
    bool is_max_min_failure   = (data.failure_maxima_set && item.failure_min     == data.max_failure_min);

    bool is_max_total_interrupted = (data.interrupted_maxima_set && item.interrupted_total   == data.max_interrupted_total);
    bool is_max_avg_interrupted   = (data.interrupted_maxima_set && item.interrupted_average == data.max_interrupted_average);
    bool is_max_max_interrupted   = (data.interrupted_maxima_set && item.interrupted_max     == data.max_interrupted_max);
    bool is_max_min_interrupted   = (data.interrupted_maxima_set && item.interrupted_min     == data.max_interrupted_min);

    bool is_max_total_total = (data.total_maxima_set && item.total_total   == data.max_total_total);
    bool is_max_avg_total   = (data.total_maxima_set && item.total_average == data.max_total_average);
    bool is_max_max_total   = (data.total_maxima_set && item.total_max     == data.max_total_max);
    bool is_max_min_total   = (data.total_maxima_set && item.total_min     == data.max_total_min);

    doc.body << "            <tr" << (item.failed > 0 ? " class=\"failure\"": "") << ">" << std::endl
             << "              <td data-sort=\"" << id << "\">" << detail::escape_html(item.job_name) << "</td>" << std::endl

             << "              <td class=\"value scheduling" << (is_max_total_scheduling ? " maximum" : "") << "\" data-sort=\"" << item.scheduling_total.count()   << "\">" << detail::format_duration(item.scheduling_total)   << "</td>" << std::endl
             << "              <td class=\"value scheduling" << (is_max_avg_scheduling ? " maximum" : "")   << "\" data-sort=\"" << item.scheduling_average.count() << "\">" << detail::format_duration(item.scheduling_average) << "</td>" << std::endl
             << "              <td class=\"value scheduling" << (is_max_max_scheduling ? " maximum" : "")   << "\" data-sort=\"" << item.scheduling_max.count()     << "\">" << detail::format_duration(item.scheduling_max)     << "</td>" << std::endl
             << "              <td class=\"value scheduling" << (is_max_min_scheduling ? " maximum" : "")   << "\" data-sort=\"" << item.scheduling_min.count()     << "\">" << detail::format_duration(item.scheduling_min)     << "</td>" << std::endl
             << "              <td class=\"value scheduling\" data-sort=\"" << item.scheduled << "\">" << item.scheduled << "</td>" << std::endl

             << "              <td class=\"value execution" << (is_max_total_execution ? " maximum" : "") << "\" data-sort=\"" << item.execution_total.count()   << "\">" << detail::format_duration(item.execution_total)   << "</td>" << std::endl
             << "              <td class=\"value execution" << (is_max_avg_execution ? " maximum" : "")   << "\" data-sort=\"" << item.execution_average.count() << "\">" << detail::format_duration(item.execution_average) << "</td>" << std::endl
             << "              <td class=\"value execution" << (is_max_max_execution ? " maximum" : "")   << "\" data-sort=\"" << item.execution_max.count()     << "\">" << detail::format_duration(item.execution_max)     << "</td>" << std::endl
             << "              <td class=\"value execution" << (is_max_min_execution ? " maximum" : "")   << "\" data-sort=\"" << item.execution_max.count()     << "\">" << detail::format_duration(item.execution_min)     << "</td>" << std::endl
             << "              <td class=\"value execution\" data-sort=\"" << item.successful << "\">" << item.successful << "</td>" << std::endl

             << "              <td class=\"value failure_cell" << (is_max_total_failure ? " maximum" : "") << "\" data-sort=\"" << item.failure_total.count()   << "\">" << detail::format_duration(item.failure_total)   << "</td>" << std::endl
             << "              <td class=\"value failure_cell" << (is_max_avg_failure ? " maximum" : "")   << "\" data-sort=\"" << item.failure_average.count() << "\">" << detail::format_duration(item.failure_average) << "</td>" << std::endl
             << "              <td class=\"value failure_cell" << (is_max_max_failure ? " maximum" : "")   << "\" data-sort=\"" << item.failure_max.count()     << "\">" << detail::format_duration(item.failure_max)     << "</td>" << std::endl
             << "              <td class=\"value failure_cell" << (is_max_min_failure ? " maximum" : "")   << "\" data-sort=\"" << item.failure_max.count()     << "\">" << detail::format_duration(item.failure_min)     << "</td>" << std::endl
             << "              <td class=\"value failure_cell\" data-sort=\"" << item.failed << "\">" << item.failed << "</td>" << std::endl

             << "              <td class=\"value interrupted" << (is_max_total_interrupted ? " maximum" : "") << "\" data-sort=\"" << item.interrupted_total.count()   << "\">" << detail::format_duration(item.interrupted_total)   << "</td>" << std::endl
             << "              <td class=\"value interrupted" << (is_max_avg_interrupted ? " maximum" : "")   << "\" data-sort=\"" << item.interrupted_average.count() << "\">" << detail::format_duration(item.interrupted_average) << "</td>" << std::endl
             << "              <td class=\"value interrupted" << (is_max_max_interrupted ? " maximum" : "")   << "\" data-sort=\"" << item.interrupted_max.count()     << "\">" << detail::format_duration(item.interrupted_max)     << "</td>" << std::endl
             << "              <td class=\"value interrupted" << (is_max_min_interrupted ? " maximum" : "")   << "\" data-sort=\"" << item.interrupted_max.count()     << "\">" << detail::format_duration(item.interrupted_min)     << "</td>" << std::endl
             << "              <td class=\"value interrupted\" data-sort=\"" << item.interrupted << "\">" << item.interrupted << "</td>" << std::endl

             << "              <td class=\"value total" << (is_max_total_total ? " maximum" : "") << "\" data-sort=\"" << item.total_total.count() << "\">"   << detail::format_duration(item.total_total)   << "</td>" << std::endl
             << "              <td class=\"value total" << (is_max_avg_total ? " maximum" : "")   << "\" data-sort=\"" << item.total_average.count() << "\">" << detail::format_duration(item.total_average) << "</td>" << std::endl
             << "              <td class=\"value total" << (is_max_max_total ? " maximum" : "")   << "\" data-sort=\"" << item.total_max.count() << "\">"     << detail::format_duration(item.total_max)     << "</td>" << std::endl
             << "              <td class=\"value total" << (is_max_min_total ? " maximum" : "")   << "\" data-sort=\"" << item.total_max.count() << "\">"     << detail::format_duration(item.total_min)     << "</td>" << std::endl
             << "              <td class=\"value total\" data-sort=\"" << item.count << "\">" << item.count << "</td>" << std::endl

             << "              <td class=\"noborder\">" << std::endl
    // Here, we only hide the button so that the row height remains equal.
             << "                <button type=\"button\" onclick=\"copyToOverlay(this.parentNode.getElementsByTagName('div')[0]);\"" << (detail.has_detail ? "" : " style=\"visibility: hidden;\"") << ">" << std::endl
             << detail_button
             << "                </button>" << std::endl
             << "                <div class=\"detail_view\">" << std::endl
             << "                  <h3>Individual jobs</h3>" << std::endl;
    detail.generate_table(doc);
    doc.body << "                </div>" << std::endl
             << "              </td>" << std::endl
             << "              <td class=\"noborder\">" << std::endl;
    if (detail.has_histograms) {
        doc.body << "                <button type=\"button\" onclick=\"copyToOverlay(this.parentNode.getElementsByTagName('div')[0]);\">" << std::endl
                 << histogram_button
                 << "                </button>" << std::endl
                 << "                <div class=\"detail_view\">" << std::endl
                 << "                  <h3>Histograms</h3>" << std::endl;
        detail.generate_histograms(doc);
        doc.body << "                </div>" << std::endl;
    }
    doc.body << "              </td>" << std::endl
             << "            </tr>" << std::endl;
}

// Ends a full table
void end_table(document & doc) {
    doc.body << "          </tbody>" << std::endl
             << "        </table>"   << std::endl
             << "      </div>"       << std::endl;
}

}

// Function for formatting diagnostics types.
// Placed outside of the 'formatter' class to allow template specialization.
template <typename DataType>
void format(document & /* doc */, std::size_t index, std::string const& section, parameters const& /* params */, DataType data) {
    // static_assert does not work here, because the compiler may generate code for this method even if it is not used.
    throw std::logic_error(std::string("Cannot format data of type ") + typeid(data).name() + " (in section '" + section + "' of index " + std::to_string(index) + ")");
}

// Formatting specializations for default diagnostics types

// Diagnostics for currently running job
template <>
void format<scheduler_diagnostics::current_type>(document & doc, std::size_t /* index */, std::string const& section, parameters const& /* params */, scheduler_diagnostics::current_type data) {
    // Add heading
    doc.body << "      <h4>" << detail::escape_html(section) << "</h4>" << std::endl;

    // Add table header
    doc.body << "      <table class=\"sortable\">"                                                            << std::endl
             << "        <thead>"                                                                             << std::endl
             << "          <tr>"                                                                              << std::endl
             << "            <th data-column=\"0\">Job name</th>"                                             << std::endl
             << "            <th class=\"scheduling\" data-column=\"1\">Scheduling time (s.ms.&micro;s)</th>" << std::endl
             << "            <th class=\"execution\" data-column=\"2\">Execution time (s.ms.&micro;s)</th>"   << std::endl
             << "            <th class=\"total\" data-column=\"3\">Total time (s.ms.&micro;s)</th>"           << std::endl
             << "          </tr>"                                                                             << std::endl
             << "        </thead>"                                                                            << std::endl
             << "        <tbody>"                                                                             << std::endl;

    // Get the current time
    auto now = boost::chrono::high_resolution_clock::now();

    // Maximum values for font coloring

    boost::chrono::nanoseconds max_scheduling;
    boost::chrono::nanoseconds max_execution;
    boost::chrono::nanoseconds max_total;

    bool extrema_set = false;

    // Process each item (but do not add them yet) to generate cell values and calculate the maximum
    std::vector<simple_diagnostic_item> processed;
    for (auto & item : data) {
        if (item.first.empty()) continue;

        // Get times
        boost::chrono::nanoseconds scheduling = item.second.get_started_time() - item.second.get_posted_time();
        boost::chrono::nanoseconds execution = now - item.second.get_started_time(); // Use current time for running tasks.
        boost::chrono::nanoseconds total = scheduling + execution;

        // Set maxima
        if (!extrema_set || scheduling > max_scheduling) max_scheduling = scheduling;
        if (!extrema_set || execution > max_execution) max_execution = execution;
        if (!extrema_set || total > max_total) max_total = total;
        extrema_set = true;

        processed.push_back(simple_diagnostic_item{item.first, scheduling, execution, total, false, false});
    }

    // Write HTML
    for (std::size_t id = 0; id < processed.size(); ++id) {
        auto & item = processed[id];

        // Maximum values have differing font color
        bool is_scheduling_maximum = (item.scheduling == max_scheduling);
        bool is_execution_maximum = (item.execution == max_execution);
        bool is_total_maximum = (item.total == max_total);

        // Add table row
        doc.body << "          <tr>" << std::endl
                 << "            <td data-sort=\"" << id << "\">" << detail::escape_html(item.job_name) << "</td>" << std::endl
                 << "            <td class=\"value scheduling" << (is_scheduling_maximum ? " maximum" : "") << "\" data-sort=\"" << item.scheduling.count() << "\">" << detail::format_duration(item.scheduling) << "</td>" << std::endl
                 << "            <td class=\"value execution"  << (is_execution_maximum ? " maximum" : "")  << "\" data-sort=\"" << item.execution.count()  << "\">" << detail::format_duration(item.execution)  << "</td>" << std::endl
                 << "            <td class=\"value total"      << (is_total_maximum ? " maximum" : "")      << "\" data-sort=\"" << item.total.count()      << "\">" << detail::format_duration(item.total)      << "</td>" << std::endl
                 << "          </tr>" << std::endl;
    }

    // Finish table
    doc.body << "        </tbody>" << std::endl
             << "      </table>"   << std::endl;
}

// All diagnostics
template <>
void format<scheduler_diagnostics>(document & doc, std::size_t /* index */, std::string const& section, parameters const& params, scheduler_diagnostics data) {
    // Add heading
    doc.body << "      <h4>" << section << "</h4>" << std::endl;

    // Collect data

    std::map<std::string, std::vector<simple_diagnostic_item>> simple_items;
    summary_diagnostics summary(data, simple_items);

    // Add table
    detail::begin_table(doc, params, summary.has_fails, summary.has_interrupts);

    std::size_t id = 0;
    for (auto it = summary.items.begin(); it != summary.items.end(); ++it, ++id) {
        detail::add_row(doc, it->second, summary, std::move(detail::row_detail(params, it->second, simple_items[it->first])), id);
    }

    detail::end_table(doc);
}

// Summary diagnostics
template <>
void format<summary_diagnostics>(document & doc, std::size_t /* index */, std::string const& section, parameters const& params, summary_diagnostics data) {
    // Add heading
    doc.body << "      <h4>" << section << "</h4>" << std::endl;

    // Add table
    detail::begin_table(doc, params, data.has_fails, data.has_interrupts);

    std::size_t id = 0;
    for (auto it = data.items.begin(); it != data.items.end(); ++it, ++id) {
        detail::add_row(doc, it->second, data, std::move(detail::row_detail(params, it->second)), id);
    }

    detail::end_table(doc);

}

// No diagnostics - do not show output
template <>
void format<disable_diagnostics>(document & /* doc */, std::size_t /* index */, std::string const& /* section */, parameters const& /* params */, disable_diagnostics /* data */) { /* No diagnostics, no output. */ }

// Diagnostic types must be default-constructible and copy-assignable
// Diagnostic types must offer 'merge(boost::asynchronous::scheduler_diagnostics)'
template <typename Current = scheduler_diagnostics,
          typename All = summary_diagnostics>
class formatter {
private:
    std::vector<Current> m_current_diagnostics;
    std::vector<All> m_all_diagnostics;

    std::vector<scheduler_interface> m_interfaces;

    parameters m_params;

public:
    typedef parameters parameter_type;

    // Constructors

    formatter(parameters params = parameters())
        : m_params(std::move(params))
    {}

    formatter(std::vector<boost::asynchronous::scheduler_interface> interfaces,
              parameters params = parameters())
        : m_interfaces(std::move(interfaces))
        , m_params(std::move(params))
    {}

    // Formatting of individual data blocks

    // Add a menu entry and a section heading
    template <typename NameType = std::string>
    void menu(document & doc, std::size_t index, NameType const& name_) {
        // Handle empty names, escape HTML strings
        NameType name = name_;
        if (name == "") name = "Scheduler " + std::to_string(index) + " (unnamed)";
        name = detail::escape_html(name);

        // Add entry to menu
        doc.header << "        <li onclick=\"this.children[0].click();\"><a href=\"#" << index << "\">" << name << "</a></li>" << std::endl;

        // Add heading
        doc.body << "      <h2 id=\"" << index << "\">" << name << "</h2>" << std::endl;
    }

    // Add the queue sizes to the document
    template <typename QueueSizeType = std::vector<std::size_t>>
    void queues(document & doc, std::size_t /* index */, QueueSizeType const& queue_sizes) {
        // Only add information if there are any queues
        if (queue_sizes.size() > 0) {
            // Add heading with the proper singular/plural form
            doc.body << "      <h4>Queue size" << (queue_sizes.size() == 1 ? "" : "s") << "</h4>" << std::endl
                     << "      <table class=\"queues\">"                                          << std::endl;

            // Add queue sizes
            for (std::size_t size : queue_sizes) {
                doc.body << "        <tr>"                                               << std::endl
                         << "          <td class=\"value noborder\">" << size << "</td>" << std::endl
                         << "        </tr>"                                              << std::endl;
            }
            doc.body << "      </table>" << std::endl;
        }
    }

    // Add diagnostics to the document.
    template <typename DataType>
    void format(document & doc, std::size_t index, std::string const& section, DataType data) {
        using boost::asynchronous::html_formatter::format; // ADL
        format<DataType>(doc, index, section, m_params, std::forward<DataType>(data));
    }

    // Formatting

    template <typename NamesType      = std::vector<std::string> &&,
              typename QueueSizesType = std::vector<std::vector<std::size_t>> &&,
              typename RunningType    = std::vector<scheduler_diagnostics::current_type> &&,
              typename CurrentType    = std::vector<Current> &&,
              typename AllType        = std::vector<All> &&>
    std::string format(std::size_t count, NamesType names, QueueSizesType queue_sizes, RunningType running, CurrentType current, AllType all) {
        document doc(m_params);

        for (std::size_t index = 0; index < count; ++index) {
            // Add menu entry
            menu  (doc, index,            names[index]);
            // Format running jobs
            format(doc, index, "Running", running[index]);
            // Format queue sizes
            queues(doc, index,            queue_sizes[index]);
            // Format current jobs
            format(doc, index, "Current", current[index]);
            // Format all jobs
            format(doc, index, "All",     all[index]);
        }

        return doc.str();
    }

    std::string format() {
        // Fetch new diagnostics from the current schedulers
        std::vector<scheduler_diagnostics> diagnostics(m_interfaces.size());
        std::vector<std::vector<std::size_t>> queue_sizes(m_interfaces.size());
        std::vector<std::string> names(m_interfaces.size());

        for (std::size_t index = 0; index < m_interfaces.size(); ++index) {
            diagnostics[index] = m_interfaces[index].get_diagnostics();
            queue_sizes[index] = m_interfaces[index].get_queue_sizes();
            names[index] = m_interfaces[index].name;
        }

        // Clone the current diagnostics and merge them with the new data.
        // Also, extract the information on running jobs
        std::vector<Current> current(diagnostics.size());
        std::vector<All> all(diagnostics.size());
        std::vector<scheduler_diagnostics::current_type> running(diagnostics.size());

        for (std::size_t index = 0; index < m_current_diagnostics.size(); ++index) {
            current[index] = m_current_diagnostics[index];
            all[index] = m_all_diagnostics[index];
        }
        for (std::size_t index = 0; index < current.size(); ++index) {
            running[index] = diagnostics[index].current();
            current[index].merge(diagnostics[index]);
            all[index].merge(std::move(diagnostics[index]));
        }

        // Format 'running', 'current' and 'm_all_diagnostics'
        return format(diagnostics.size(), std::move(names), std::move(queue_sizes), std::move(running), std::move(current), std::move(all));
    }

    // Clearing data

    void clear_schedulers() {
        std::vector<scheduler_diagnostics> diagnostics(m_interfaces.size());
        // Separate loops to make sure the diagnostics are fetched as closely to one another as possible
        // Of course, this does not prohibit the compiler from joining the loops, but it may serve as a hint...
        for (std::size_t index = 0; index < m_interfaces.size(); ++index) {
            diagnostics[index] = m_interfaces[index].clear();
        }
        // Resize storage as needed
        if (m_current_diagnostics.size() < diagnostics.size()) m_current_diagnostics.resize(diagnostics.size());
        if (m_all_diagnostics.size() < diagnostics.size()) m_all_diagnostics.resize(diagnostics.size());
        // Merge data into the local storage
        for (std::size_t index = 0; index < diagnostics.size(); ++index) {
            m_current_diagnostics[index].merge(diagnostics[index]);
            m_all_diagnostics[index].merge(std::move(diagnostics[index]));
        }
    }

    void clear_current() {
        m_current_diagnostics.clear();
    }

    void clear_all() {
        m_current_diagnostics.clear();
        m_all_diagnostics.clear();
    }
};

}

}}

#endif // BOOST_ASYNC_HTML_FORMATTER_HPP
