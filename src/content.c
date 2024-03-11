#include "nexus.h"
#include "cmd.h"

int content_build_math(Nexus *nexus, Node *anchor) /*{{{*/
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_MATH, CMD_NONE, "Math", "", NODE_LEAF);

    NEXUS_INSERT(nexus, &base, &sub, ICON_MATH, CMD_NONE, "Operations", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Addition", "Common terms used:\nsum = term + term\nsum = summand + summand\nsum = addend + addend\nsum = augend + addend", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Subtraction", "Common terms used:\ndifference = term - term\ndifference = minuend - subtrahend", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Multiplication", "Common terms used:\nproduct = factor * factor\nproduct = multiplier * multiplicand", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Division", "Common terms used:\nfraction or quotient or ratio = dividend / divisor\nfraction or quotient or ratio = numerator / denominator", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Exponentiation", "Common terms used:\npower = base ^ exponent", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "nth root", "Common terms used:\n"
            "       degree /--------\n"
            "root =      \\/ radicand", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Logarithm", "Common terms used:\nlogarithm = log_{base} ( anti-logarithm )", NODE_LEAF);
    return 0;
error:
    return -1;
} /*}}}*/

int content_build_physics(Nexus *nexus, Node *anchor) //{{{
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_PHYSICS, CMD_NONE, "Physics", "", NODE_LEAF);

    /* set up units */
    NEXUS_INSERT(nexus, &base, &sub, ICON_PHYSICS, CMD_NONE, "Physical Units", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Second", "A second is an SI-unit of time.", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Meter", "A meter is an SI-unit of length.", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Speed of Light", "The speed of light " F("in vacuum", IT) " is " F("c = 299'792'458 meters/second", BOLD UL) "", "Second", "Meter");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Volt", "Volt has the unit [V]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Ohm", "Ohm has the unit [Ω]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Ampere", "Ampere has the unit [A]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Coulomb", "Coulomb has the unit [C]", NODE_LEAF);

    /* set up other stuff */
    NEXUS_INSERT(nexus, &base, &sub, ICON_PHYSICS, CMD_NONE, "Electrical Engineering", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Voltage", "A common formula for the voltage U is " F("U = R * I", BOLD UL) ", where ..\n"
            "  - R is the resistance in Ohm\n"
            "  - I is the electric current in Ampere", "Volt");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Resistance", "A common formula for the resistance R is " F("R = U / I", BOLD UL) ", where ..\n"
            "  - U is the voltage in Volt\n"
            "  - I is the electric current in Ampere", "Ohm", "Voltage");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Electric Current", "A common formula for the electric current I is " F("I = U / R", BOLD UL) ", where ..\n"
            "  - I is the electric current in Ampere\n"
            "  - R is the resistance in Ohm", "Voltage", "Ampere", "Resistance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Capacitance", "A common formula for the capacitance C is " F("C = Q / U", BOLD UL) ", where ..\n"
            "  - Q is the charge in Coulomb\n"
            "  - U is the voltage in Volts", "Coulomb", "Voltage");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Capacitor", "In electrical engineering, a " F("capacitor", BOLD) " is a device that stores electrical energy by accumulating electric charges on two closely spaced surfaces that are insulated from each other. It is a passive electronic component with two terminals.\n\n"
            "The effect of a capacitor is known as " F("capacitance", UL) ". While some capacitance exists between any two electrical conductors in proximity in a circuit, a capacitor is a component designed to add capacitance to a circuit. The capacitor was originally known as the condenser, a term still encountered in a few compound names, such as the condenser microphone.", "Capacitance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Resistor", "A resistor is a passive two-terminal electrical component that implements electrical resistance as a circuit element. In electronic circuits, resistors are used to reduce current flow, adjust signal levels, to divide voltages, bias active elements, and terminate transmission lines, among other uses.\n\n"
            "The electrical function of a resistor is specified by its " F("resistance", UL), "Resistance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Ohm's law", "Ohm's law is basically any of the common formulas in " F("Resistance", UL) ", " F("Electric Current", UL) " or " F("Capacitance", UL), "Resistance", "Capacitance", "Electric Current");

    return 0;
error:
    return -1;
} //}}}

int content_build_controls(Nexus *nexus, Node *anchor) /*{{{*/
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_WIKI, CMD_NONE, "Controls", "Guide to all the various controls in c-nexus." , NODE_LEAF);

    /* normal view {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Normal View", "In this mode you can browse the notes.\n"
            "\n" F("basic controls", UL) "\n"
            "  for the normal mode are listed in the root, so I'm not going to list them again.\n"
            "\n" F("other controls", UL) "\n"
            "  H                : go back to most recent search\n"
            "  f                : enter " F("search mode", FG_YL_B) "\n"
            "  t                : enter " F("icon view", FG_YL_B) "\n"
            "  q                : quit and return to the terminal\n"
            "  Q                : rebuild nexus\n"
            "  c                : run command associated to current note\n"
            "  C                : run command associated to note pointed at by the arrow\n"
            "  SPACE            : toggle showing note descriptions on/off\n"
            "  i                : toggle previewing note descriptions on/off", NODE_LEAF);
    /* }}} */

    /* search view {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Search View", "In this mode you can search notes for substrings.\n"
            "\n" F("what gets searched?", UL) "\n"
            "  the exact pattern of each note you can search are (in this order): " F("ICON title command description", FG_GN) " (e.g. search for: 'wiki normal' -> you will find normal view note)\n"
            "  searches are case insensitive. any newlines are removed. whitespaces are condensed into one space (e.g. XYZ    ABC -> XYZ ABC)\n"
            "\n" F("controls while editing search string", UL) "\n"
            "  ESC              : browse found notes\n"
            "  ENTER            : browse found notes\n"
            "  [type anything]  : search notes for substring\n"
            "  CTRL+BACK        : erase a word/until word\n"
            "\n" F("controls while browsing found notes", UL) "\n"
            "  ESC              : abort search and go back to " F("normal view", FG_YL_B) "\n"
            "  ENTER            : edit search string\n"
            "  f                : same as above\n"
            "  F                : clear search string and edit it\n"
            "  t                : enter " F("icon view", FG_YL_B) "\n"
            "  hjkl             : same as the basic controls\n"
            "  H                : go back to most recent search\n"
            "  q                : quit and return to the terminal\n"
            "  Q                : rebuild nexus\n"
            "  C                : run command associated to note pointed at by the arrow\n"
            "  i                : toggle previewing note descriptions on/off\n"
            , "Normal View");
    /* }}} */

    /* icon view {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Icon View", "In this mode the notes are browsable by icon.\n"
            "  f                : enter " F("search mode", FG_YL_B) "\n"
            "  t, ESC           : abort icon view and go back\n"
            "  hjkl             : same as the basic controls\n"
            , "Normal View", "Search View");
    /* }}} */

    return 0;
error:
    return -1;
} /*}}}*/

int content_build_cmds(Nexus *nexus, Node *anchor) /* {{{ */
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(anchor, ERR_NULL_ARG);
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_WIKI, "", "Notes with Commands", "Every note can have a command attached. See controls on how to use it.", "Controls");
    /* linux {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Linux Commands", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_WIKI, CMD_SUDO("shutdown -h now"), "Shutdown linux", "This command lets you shut down your linux system", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_WIKI, CMD_SUDO("reboot"), "Reboot linux", "This command lets you reboot your linux system", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_WIKI, CMD_IMG("images/stickman.png"), "A stickman", "Execute the command on this note to show an image with the program imv!", NODE_LEAF);
    /* }}} */
    /* windows {{{ */
    /* tbd }}} */
    return 0;
error:
    return -1;
} /* }}} */

int content_build(Nexus *nexus, Node *root) //{{{
{
    TRY(content_build_controls(nexus, root), "failed building controls");
    TRY(content_build_cmds(nexus, root), "failed building cmds");
    TRY(content_build_physics(nexus, root), "failed building physics");
    TRY(content_build_math(nexus, root), "failed building math");
    return 0;
error:
    return -1;
} //}}}

