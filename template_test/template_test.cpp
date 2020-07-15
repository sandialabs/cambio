#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

using namespace std;
using boost::filesystem::path;
namespace bp = boost::process;

int generateOutput(path inputFile, path inputTemplate, path outputFile) {

	// This is built by the cambio project
	path cl("../Debug/cambio.exe");

	string commandString;
	commandString.append(cl.string());
	commandString.append(" --input=");
	commandString.append(inputFile.string());
	commandString.append(" --output=");
	commandString.append(outputFile.string());
	commandString.append(" --template-file=");
	commandString.append(inputTemplate.string());

	return bp::system(commandString);
}

int testChannelDataTemplate(path input_directory, path output_directory) {
	cout << "TEST: channel data template ... ";

	path expectedOutput = output_directory / path("TEST_channel_data.n42");

	int result = generateOutput(
		input_directory / path("pu239_1C_Detective_X_50cm.pcf"),
		input_directory / path("TEST_channel_data.n42"),
		expectedOutput);

	// Load and check output file
	if (result != 0) {
		cout << "FAILED, error running process" << endl;
		return EXIT_FAILURE;
	}

	if (!boost::filesystem::exists(expectedOutput)) {
		cout << "FAILED, file not found" << endl;
		return EXIT_FAILURE;
	}

	string absOutputPath = boost::filesystem::absolute(expectedOutput).string();
	rapidxml::file<> xmlFile(absOutputPath.c_str());
	rapidxml::xml_document<> doc;
	doc.parse<0>(xmlFile.data());

	rapidxml::xml_node<>* node = doc.first_node("ChannelData");
	string checkValue = string(node->value());
	
	if (checkValue.length() != 23270) {
		cout << "FAILED, data incorrect length: " << checkValue.length() << endl;
		return EXIT_FAILURE;
	}

	cout << "SUCCESS" << endl;

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	cout << "Cambio Template Test Utility" << endl;

	// This is obviously only valid if the build directory is at the cambio repository root
	path input_directory("../../../Templates/Test/");
	path output_directory("./template_test_output/");

	// Clean output directory and recreate
	cout << "Cleaning and recreating output directory..." << endl;
	boost::filesystem::remove_all(output_directory);
	boost::filesystem::create_directory(output_directory);

	int testCount = 0;
	int failureCount = 0;

	failureCount += testChannelDataTemplate(input_directory, output_directory);
	testCount++;

	cout << endl;
	cout << "Test Summary" << endl;
	cout << (testCount - failureCount) << " tests passed of " << testCount << endl;

	return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}//int main( int argc, char **argv )



