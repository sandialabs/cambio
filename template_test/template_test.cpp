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

int compareFiles(const std::string& p1, const std::string& p2) {
	std::ifstream f1(p1);
	std::ifstream f2(p2);

	if (f1.fail() || f2.fail()) {
		return false; //file problem
	}

	string line1, line2;
	int lineCount = 0;
	while (getline(f1, line1) && getline(f2, line2)) {

		if (lineCount == 0) {
			// inja eats the BOM in the input file if it exists, so we have to do the same here to match outputs
			if (line1.rfind("\xEF\xBB\xBF", 0) == 0) {
				line1 = line1.substr(3);
			}
		}

		lineCount++;

		if (line1 != line2) {
			return lineCount;
		}
	}

	if (getline(f1, line1) || getline(f2, line2)) {
		// files have different numbers of lines, so flag that
		return lineCount;
	}

	return -1;
}

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

int test_Aspect_MKC(path template_directory, path output_directory) {
	cout << "TEST: Aspect MKC template ... ";

	path expectedOutput = output_directory / path("Aspect MKC.spc");
	path inputFile = template_directory / path("Test") / path("Aspect_MKC") / path("s0044.spc");

	if (false /* this one should fail! */ == generateOutput(
		inputFile,
		template_directory / path("Aspect MKC") / path("TEMPLATE_s0044.spc"),
		expectedOutput) != 0)
	{
		cout << "Template should have failed because this is a binary file" << endl;
		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}

int test_AtomTex_6101C(path template_directory, path output_directory) {
	cout << "TEST: AtomTex 6101C SPE template ... ";

	path expectedOutput = output_directory / path("AtomTex_6101C_spec_20170131_151620.spe");
	path inputFile = template_directory / path("Test") / path("AtomTex_6101C") / path("spec_20170131_151620.spe");

	if (generateOutput(
		inputFile,
		template_directory / path("AtomTex_6101C") / path("TEMPLATE_spec_20170131_151620.spe"),
		expectedOutput) != 0)
	{
		return EXIT_FAILURE;
	}

	//TESTCODE
	generateOutput(
		inputFile,
		template_directory / path("Test") / path("TEMPLATE_ALL.txt"),
		output_directory / path("AtomTex_6101C_spec_20170131_151620_all.txt")
	);

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

int test_FLIRR425(path template_directory, path output_directory) {
	cout << "TEST: FLIR R425 template ... ";

	path expectedOutput = output_directory / path("FLIR R425 Spectra_599.n42");
	path inputFile = template_directory / path("Test") / path("FLIR_R425") / path("Spectra_599.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("FLIR_R425") / path("TEMPLATE_Spectra_599.n42"),
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

int test_Ortec_Detective_X(path template_directory, path output_directory) {
	cout << "TEST: Ortec Detective X template ... ";

	path expectedOutput = output_directory / path("Ortec Detective X 0001220_2018_03_08_23_24_200.n42");
	path inputFile = template_directory / path("Test") / path("Ortec_Detective_X") / path("0001220_2018_03_08_23_24_200.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("Ortec_Detective_X") / path("TEMPLATE_0001220_2018_03_08_23_24_200.n42"),
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

int test_Ortec_RadEagleT_With_Intrinsic_Source(path template_directory, path output_directory) {
	cout << "TEST: Ortec RadEagle T (w/ intrinsic source) template ... ";

	path expectedOutput = output_directory / path("Ortec RadEagleT wSource 2019-11-25T12-50-11_161.n42");
	path inputFile = template_directory / path("Test") / path("Ortec_RadEagleT") / path("with_intrinsic_source") / path("2019-11-25T12-50-11_161.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("Ortec_RadEagleT") / path("with_intrinsic_source") / path("TEMPLATE_2019-11-25T12-50-11_161.n42"),
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

int test_Ortec_RadEagleT_No_Intrinsic_Source(path template_directory, path output_directory) {
	cout << "TEST: Ortec RadEagle T (NO intrinsic source) template ... ";

	path expectedOutput = output_directory / path("Ortec RadEagleT No Source 2019-11-25T12-15-44_223.n42");
	path inputFile = template_directory / path("Test") / path("Ortec_RadEagleT") / path("No_intrinsic_source") / path("2019-11-25T12-15-44_223.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("Ortec_RadEagleT") / path("No_intrinsic_source") / path("TEMPLATE_2019-11-25T12-15-44_223.n42"),
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

int test_Polimaster_1410(path template_directory, path output_directory) {
	cout << "TEST: Polimaster 1410 template ... ";

	path expectedOutput = output_directory / path("Polimaster 1410 201803080064.spe");
	path inputFile = template_directory / path("Test") / path("Polimaster_1410") / path("201803080064.spe");

	if (generateOutput(
		inputFile,
		template_directory / path("Polimaster_1410") / path("TEMPLATE_201803080064.spe"),
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

int test_RadSeeker(path template_directory, path output_directory) {
	cout << "TEST: RadSeeker template ... ";

	path expectedOutput = output_directory / path("RadSeeker_CS_76886_D20180308_T145826_E1227_U.n42");
	path inputFile = template_directory / path("Test") / path("RadSeeker") / path("RadSeeker_CS_76886_D20180308_T145826_E1227_U.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("RadSeeker") / path("TEMPLATE_RadSeeker_CS_76886_D20180308_T145826_E1227_U.n42"),
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

int test_Symetrica_Verifinder(path template_directory, path output_directory) {
	cout << "TEST: Symetrica Verifinder template ... ";

	path expectedOutput = output_directory / path("Symetrica Verifinder Event_SN23N-190007_D20191125_T200611Z_E0586.n42");
	path inputFile = template_directory / path("Test") / path("Symetrica_Verifinder") / path("Event_SN23N-190007_D20191125_T200611Z_E0586.n42");

	if (generateOutput(
		inputFile,
		template_directory / path("Symetrica_Verifinder") / path("TEMPLATE_Event_SN23N-190007_D20191125_T200611Z_E0586.n42"),
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

int test_Thermo_RIIDEye(path template_directory, path output_directory) {
	cout << "TEST: Thermo RIIDEye template ... ";

	path expectedOutput = output_directory / path("Thermo SPEC0654.N42");
	path inputFile = template_directory / path("Test") / path("Thermo_RIIDEyeX") / path("SPEC0654.N42");

	if (generateOutput(
		inputFile,
		template_directory / path("Thermo_RIIDEyeX") / path("TEMPLATE_SPEC0654.N42"),
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

	try {
		boost::filesystem::remove_all(output_directory);
		boost::filesystem::create_directory(output_directory);
	}
	catch (exception&) {}

	int testCount = 0;
	int failureCount = 0;

	failureCount += testChannelDataTemplate(template_directory, output_directory); testCount++;
	failureCount += testChannelDataCompressionTemplate(template_directory, output_directory); testCount++;
	failureCount += testTimesTemplate(template_directory, output_directory); testCount++;
	//failureCount += test_Aspect_MKC(template_directory, output_directory); testCount++; // This one is a binary file that fails anyway
	failureCount += test_AtomTex_6101C(template_directory, output_directory); testCount++;
	failureCount += test_FLIRR400_2006(template_directory, output_directory); testCount++;
	failureCount += test_FLIRR400_2012(template_directory, output_directory); testCount++;
	failureCount += test_FLIRR425(template_directory, output_directory); testCount++;
	failureCount += test_Ortec_Detective_X(template_directory, output_directory); testCount++;
	failureCount += test_Ortec_RadEagleT_With_Intrinsic_Source(template_directory, output_directory); testCount++;
	failureCount += test_Ortec_RadEagleT_No_Intrinsic_Source(template_directory, output_directory); testCount++;
	failureCount += test_Polimaster_1410(template_directory, output_directory); testCount++;
	failureCount += test_RadSeeker(template_directory, output_directory); testCount++;
	failureCount += test_Symetrica_Verifinder(template_directory, output_directory); testCount++; // Note this one has like a billion spectra in it, only did the first few...
	failureCount += test_Thermo_RIIDEye(template_directory, output_directory); testCount++;

	cout << endl;
	cout << "Test Summary" << endl;
	cout << (testCount - failureCount) << " tests passed of " << testCount << endl;

	return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}//int main( int argc, char **argv )



