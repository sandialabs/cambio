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
	commandString.append("\"" + inputFile.string() + "\"");
	commandString.append(" --output=");
	commandString.append("\"" + outputFile.string() + "\"");
	commandString.append(" --template-file=");
	commandString.append("\"" + inputTemplate.string() + "\"");

	cout << commandString << endl;

	int result = bp::system(commandString);

	if (result != 0) {
		cout << "FAILED, error running process" << endl;
		return EXIT_FAILURE;
	}

	if (!boost::filesystem::exists(outputFile)) {
		cout << "FAILED, file not found" << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int testChannelDataTemplate(path template_directory, path output_directory) {
	cout << "TEST: channel data template ... ";

	path expectedOutput = output_directory / path("TEST_channel_data.n42");

	if (generateOutput(
		template_directory / path("Test") / path("pu239_1C_Detective_X_50cm.pcf"),
		template_directory / path("Test") / path("TEST_channel_data.n42"),
		expectedOutput) != 0) 
	{
		return EXIT_FAILURE;
	}

	// Load and check output file
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

int testChannelDataCompressionTemplate(path template_directory, path output_directory) {
	cout << "TEST: compressed channel data template ... ";

	path expectedOutput = output_directory / path("TEST_channel_data_compressed.n42");

	if (generateOutput(
		template_directory / path("Test") / path("pu239_1C_Detective_X_50cm.pcf"),
		template_directory / path("Test") / path("TEST_channel_data_compressed.n42"),
		expectedOutput) != 0)
	{
		return EXIT_FAILURE;
	}

	string absOutputPath = boost::filesystem::absolute(expectedOutput).string();
	rapidxml::file<> xmlFile(absOutputPath.c_str());
	rapidxml::xml_document<> doc;
	doc.parse<0>(xmlFile.data());

	rapidxml::xml_node<>* node = doc.first_node("ChannelData");
	string checkValue = string(node->value());

	if (checkValue.length() != 24240) {
		cout << "FAILED, data incorrect length: " << checkValue.length() << endl;
		return EXIT_FAILURE;
	}

	string checkCompressionCode = string(node->first_attribute("compressionCode")->value());
	if (checkCompressionCode != "CountedZeros") {
		cout << "FAILED, incorrect compression code: " << checkCompressionCode << endl;
		return EXIT_FAILURE;
	}

	cout << "SUCCESS" << endl;

	return EXIT_SUCCESS;
}

int testTimesTemplate(path template_directory, path output_directory) {
	cout << "TEST: time formatting template ... ";

	path expectedOutput = output_directory / path("TEST_times.xml");

	if (generateOutput(
		template_directory / path("Test") / path("pu239_1C_Detective_X_50cm.pcf"),
		template_directory / path("Test") / path("TEST_times.xml"),
		expectedOutput) != 0)
	{
		return EXIT_FAILURE;
	}

	string absOutputPath = boost::filesystem::absolute(expectedOutput).string();
	rapidxml::file<> xmlFile(absOutputPath.c_str());
	rapidxml::xml_document<> doc;
	doc.parse<0>(xmlFile.data());

	rapidxml::xml_node<>* node = doc.first_node("Test1");
	string checkValue = string(node->value());

	if (checkValue != "2890.6103515625") {
		cout << "FAILED, found string " << checkValue << endl;
		return EXIT_FAILURE;
	}

	node = doc.first_node("Test2");
	checkValue = string(node->value());

	if (checkValue != "3600.0") {
		cout << "FAILED, found string " << checkValue << endl;
		return EXIT_FAILURE;
	}

	node = doc.first_node("Test3");
	checkValue = string(node->value());

	if (checkValue != "2890.61") {
		cout << "FAILED, found string " << checkValue << endl;
		return EXIT_FAILURE;
	}

	node = doc.first_node("Test4");
	checkValue = string(node->value());

	if (checkValue != "PT48M10.610S") {
		cout << "FAILED, found string " << checkValue << endl;
		return EXIT_FAILURE;
	}

	node = doc.first_node("Test5");
	checkValue = string(node->value());

	if (checkValue != "PT48M10.6S") {
		cout << "FAILED, found string " << checkValue << endl;
		return EXIT_FAILURE;
	}

	cout << "SUCCESS" << endl;

	return EXIT_SUCCESS;
}

int compareFiles(const std::string& p1, const std::string& p2) {
	std::ifstream f1(p1);
	std::ifstream f2(p2);

	if (f1.fail() || f2.fail()) {
		return false; //file problem
	}

	string line1, line2;
	int lineCount = 0;
	while (getline(f1, line1) && getline(f2, line2)) {
		lineCount++;
		if (line1 != line2) {
			return lineCount;
		}
	}

	return -1;
}

int test_FLIRR400_2006(path template_directory, path output_directory) {
	cout << "TEST: FLIR R400 2006 template ... ";

	path expectedOutput = output_directory / path("FLIR R400 Identification 3916.ANSI N42.42 2006.n42");
	path inputFile = template_directory / path("Test") / path("FLIR_R400") / path("Identification 3916.ANSI N42.42 2006.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("FLIR_R400") / path("TEMPLATE_Identification 3916.ANSI N42.42 2006.n42"),
		expectedOutput) != 0)
	{
		return EXIT_FAILURE;
	}

	string absOutputPath = boost::filesystem::absolute(expectedOutput).string();
	string absInputPath = boost::filesystem::absolute(inputFile).string();

	int lineDiff = compareFiles(absInputPath, absOutputPath);
	if (lineDiff >= 0) {

		cout << "FAILED, files differ on line " << lineDiff << endl;
		return EXIT_FAILURE;
	}

	cout << "SUCCESS" << endl;

	return EXIT_SUCCESS;
}

int test_FLIRR400_2012(path template_directory, path output_directory) {
	cout << "TEST: FLIR R400 2012 template ... ";

	path expectedOutput = output_directory / path("FLIR R400 Identification 3916.ANSI N42.42 2012.n42");
	path inputFile = template_directory / path("Test") / path("FLIR_R400") / path("Identification 3916.ANSI N42.42 2012.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("FLIR_R400") / path("TEMPLATE_Identification 3916.ANSI N42.42 2012.n42"),
		expectedOutput) != 0)
	{
		return EXIT_FAILURE;
	}

	string absOutputPath = boost::filesystem::absolute(expectedOutput).string();
	string absInputPath = boost::filesystem::absolute(inputFile).string();

	int lineDiff = compareFiles(absInputPath, absOutputPath);
	if (lineDiff >= 0) {

		cout << "FAILED, files differ on line " << lineDiff << endl;
		return EXIT_FAILURE;
	}

	cout << "SUCCESS" << endl;

	return EXIT_SUCCESS;
}


int main(int argc, char** argv)
{
	cout << "Cambio Template Test Utility" << endl;

	// This is obviously only valid if the build directory is at the cambio repository root
	path template_directory("../../../Templates/");
	path output_directory("./template_test_output/");

	// Clean output directory and recreate
	cout << "Cleaning and recreating output directory..." << endl;
	boost::filesystem::remove_all(output_directory);
	boost::filesystem::create_directory(output_directory);

	int testCount = 0;
	int failureCount = 0;

	failureCount += testChannelDataTemplate(template_directory, output_directory); testCount++;
	failureCount += testChannelDataCompressionTemplate(template_directory, output_directory); testCount++;
	failureCount += testTimesTemplate(template_directory, output_directory); testCount++;
	failureCount += test_FLIRR400_2006(template_directory, output_directory); testCount++;
	failureCount += test_FLIRR400_2012(template_directory, output_directory); testCount++;

	cout << endl;
	cout << "Test Summary" << endl;
	cout << (testCount - failureCount) << " tests passed of " << testCount << endl;

	return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}//int main( int argc, char **argv )



