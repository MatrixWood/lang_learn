// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

static double NumVal;

static int gettok() {
	static int LastChar = ' ';
	while (isspace(LastChar))
		LastChar = getchar();

	if (isdigit(LastChar) || LastChar == '.') {
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), nullptr);
		std::cout << NumVal << std::endl;
		return -5;
	}
}

void main() {
	if (gettok() == -5) {
		std::cout << "return num." << std::endl;
	}

}