#include <iostream>
#include <sstream>
#include <string>
#include<unordered_set>
#include<stack>
#include <ctype.h>
#include <assert.h>
#include <limits>

/*
* represents an information unit in arithmetic expression
* a valid token is an int or one of the following chars:
* '(', ')', '+', '-', '*', '/'
*/
class Token {
private:
	char type;
	int value;
public:
	Token(const char ch) : type(ch), value(int(ch)) {}
	Token(const char ch, const int val) : type(ch), value(val) {}

	char getType() const {
		return type;
	}
	int getValue() const {
		// only when type is a digt we consider value
		if (isdigit(type))
			return value;
		else
			// we could return any number here, 
			// for type of token that is not digit value is not taken into account
			return (int)type;
	}
	Token & operator=(const Token & t) {
		this->type = t.type;
		this->value = t.value;
		return *this;
	}

	bool operator==(const Token & t) {
		return this->type == t.type && this->value == t.value;
	}

	bool operator!=(const Token & t) {
		return !(*this == t);
	}
};

/*
* some help definitions/functions 
*/
namespace AEUtils {
	const std::unordered_set<char> allowedChars = { '(', ')', '+', '-', '*', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	const std::unordered_set<char> operators = { '+', '-', '*', '/' };
	const std::unordered_set<char> notDigits = { '+', '-', '*', '/' , '(', ')' };
	const char openBr = '(';
	const char closedBr = ')';
	const char minus = '-';
	const char plus = '+';
	const char mult = '*';
	const char div = '/';
	const Token dummyToken = '|';
	bool isOperator(const char ch) {
		return operators.find(ch) != operators.end();
	}

	bool isAllowed(const char ch) {
		return allowedChars.find(ch) != allowedChars.end();
	}

	bool isNonDigit(const char ch) {
		return notDigits.find(ch) != notDigits.end();
	}

	/*
	* removes all spaces from the string
	*/
	std::string compress(const std::string & expression) {
		std::string ret = "";
		for (char ch : expression) {
			if (!isspace(ch))
				ret += ch;
		}
		return ret;
	}
};


/*
* help data structure to read tokens from input string one by one
* in a stream
*/
class TokenStream {
private:
	std::stringstream ss;
public:
	TokenStream(const std::string tokens) : ss(tokens) {}
	/*
	* returns next token after tzhe token last read
	*/
	Token nextToken() {
		if (ss.good()) {
			char ch;
			ss >> ch;
			if (!AEUtils::isAllowed(ch))
				return AEUtils::dummyToken;
			if (AEUtils::isNonDigit(ch))
				return Token(ch);
			// case digit
			// consumed char was first digit in a row of one or several digits
			// put it back into the stream and try to read whole integer
			ss.putback(ch);
			int val;
			ss >> val;
			return Token(ch, val);
		}
		// end of stream reached
		return AEUtils::dummyToken;
	}
	bool hasNextToken() const {
		return ss.good();
	}
	void putBack(Token t) {
		ss.putback(t.getType());
	}
};

/*
* check whether open braces match closed braces.
* anything like this ...((...( ..)...)) is ok
* anything like this ...((..) is not ok.
*/
bool checkBraceNesting(const std::string & str, const char openBrace, const char closedBrace) {
	std::stack<char> st;
	for (char ch : str) {
		if (ch == openBrace)
			st.push(openBrace);
		else if (ch == closedBrace) {
			if (st.empty())
				return false;
			st.pop();
		}
	}
	return st.empty();
}


/*
* check whether expression is a valid arithmetic expression
*/
class ExpressionValidator {
private:
	bool validatePair(const char prev, const char cur) const {
		// case ")digit" 
		if (prev == AEUtils::closedBr && isdigit(cur))
			return false;
		// case ")(" 
		if ((prev == AEUtils::closedBr || isdigit(prev)) && cur == AEUtils::openBr)
			return false;
		// case "()"
		if ((prev == AEUtils::openBr || AEUtils::isOperator(prev)) && cur == AEUtils::closedBr)
			return false;
		// case "+-" (tow operators without term in between)
		if (AEUtils::isOperator(prev) && AEUtils::isOperator(cur))
			return false;
		return true;
	}

public:
	bool validate(const std::string & expr) const {
		// must contain at least one digit
		if (expr.length() < 1)
			return false;
		if (!checkBraceNesting(expr, AEUtils::openBr, AEUtils::closedBr)) {
			std::cout << "Not matching braces." << std::endl;
			return false;
		}
		char prev = AEUtils::openBr;
		for (char cur : expr) {
			if (!AEUtils::isAllowed(cur)) {
				std::cout << "invalid character in input." << std::endl;
				return false;
			}
			if (!validatePair(prev, cur))
				return false;
			prev = cur;
		}
		return true;
	}
};

/*
Evaluates expression through recursive calls
evaluate() -> getTerm() -> getUnitTerm()-> evaluate()
a valid expression has following grammar:
Expression:
1)	Term, Term + Expresssion, Expression + Term, Expression - Term
Term:
2)	UnitTerm, Term * UnitTerm, Term / UnitTerm, UnitTerm * Term, UnitTerm / Term
UnitTerm:
3)	int, (Expression)

AST is build implicit in the recursive calls

Example (3 - 1)*2 + 3:
(3 - 1) evaluates to (Expression) which is UnitTerm (rule 3)
(3 - 1)*2 evaluates to Term (rule 2) 
(3 - 1)*2 + 3 evaluates to Term + Expression (rule 1)
*/
class Expression {
private:
	std::string expression;
	bool valid = false;
	bool evaluated = false;
	int value;

	/*
	* validates whether @expr is a valid arithmetic expression
	* before validation string will be 'compressed' - all empty spaces will be removed
	* so '- 1' will be reduced to '-1'
	* and '(- 2 + 1 ) * 2'  will be reduced to '(-2+1)*2'
	*/
	bool validate(const std::string & expr) {
		if (!valid) {
			std::string compressed = AEUtils::compress(expr);
			ExpressionValidator validator;
			valid = validator.validate(compressed);
		}
		return valid;
	}

	/*
	entry point for expression evaluation
	*/
	int evaluate(TokenStream & ts) {
		int val = getTerm(ts);  // Expression = Term + Expression
		Token t = ts.nextToken();
		// process until end of TokenStream
		while (t != AEUtils::dummyToken) {
			if (t.getType() == AEUtils::plus) {
				val += getTerm(ts);
				t = ts.nextToken();
			}
			else if (t.getType() == AEUtils::minus) {
				val -= getTerm(ts);
				t = ts.nextToken();
			}
			else {
				// when token is neither '+' nor '-' 
				//put it back, it is part of Term
				ts.putBack(t);
				return val;
			}
		}
		return val;
	}
	int getTerm(TokenStream & ts) {
		int val = getUnitTerm(ts);
		Token t = ts.nextToken();
		while (t != AEUtils::dummyToken) {
			if (t.getValue() == AEUtils::mult) {
				val *= getUnitTerm(ts);
				t = ts.nextToken();
			}
			else if (t.getValue() == AEUtils::div) {
				int i = getUnitTerm(ts);
				if (i == 0)
					throw std::runtime_error("division by zero!");
				else {
					val = val / i;
					t = ts.nextToken();
				}
			}
			else {
				// when token is neither '*' nor '/'
				// put it back, it is part of expression
				ts.putBack(t);
				return val;
			}
		}
		return val;
	}

	int getUnitTerm(TokenStream & ts) {
		Token t = ts.nextToken();
		// here we care about token Vaue, because it is an int
		if (isdigit(t.getType())) 
			return t.getValue();
		if (t.getType() == AEUtils::plus)
			return getUnitTerm(ts);
		if (t.getType() == AEUtils::minus)
			return -getUnitTerm(ts);
		if (t.getType() == AEUtils::openBr) {
			// open brace means (Expression) begins
			int val = evaluate(ts);
			t = ts.nextToken();
			if (t != AEUtils::closedBr)
				throw std::runtime_error("closed brace expected.");
			return val;
		}
		std::string err = "'" + expression + "' is not valid expression.";
		throw std::runtime_error(err.c_str());
	}

public:
	Expression(const std::string & exp) : expression(exp) {
		if (validate(expression))
			valid = true;
		else
			valid = false;

		evaluated = false;
	}

	bool isValid() const {
		return valid;
	}

	bool isEvaluated() const {
		return evaluated;
	}

	int evaluate() {
		if (valid) {
			if (!evaluated) {
				try {
					TokenStream tokenStream(expression);
					value = evaluate(tokenStream);
					evaluated = true;
					return value;
				}
				catch (std::exception & ex) {
					std::cout << ex.what() << std::endl;
					valid = false;
					evaluated = false;
				}
			}
			else
				return value;
		}
		else {
			std::cout << "'" << expression << "'" << " is not valid expression.";
		}
		return std::numeric_limits<int>::min();
	}


};

void evaluate(const std::string & expr) {
	Expression expression(expr);
	if (expression.isValid()) {
		int result = expression.evaluate();
		if (expression.isEvaluated()) {
			std::cout << "evaluates to " << result << std::endl;
		}
	}
	else
		std::cout << "'" << expr << "'" << " is not valid expression." << std::endl;
}
void testCheckBraceNesting();
void testValidate();
void testExpressionValidate();
void testToken();
void testTokenStream();
void testEvaluateExpression();

int main() {
	//testCheckBraceNesting();
	//testValidate();
	//testExpressionValidate();
	//testToken();
	//testTokenStream();
	//testEvaluateExpression();

	std::string prompt1 = "Please enter an arithmetic expression.\nOnly ints, '+', '-', '*', '/' , '(' and ')' are allowed. Complete input with 'Enter'.\nEnter 'x' to exit.";
	std::string prompt2 = "Next expression:";
	std::string input = "";
	std::string prompt3 = "bye bye!";
	std::cout << prompt1 << std::endl;
	while (true) {
		getline(std::cin, input);
		if (input == "X" || input == "x") {
			std::cout << prompt3 << std::endl;
			return 0;
		}
		evaluate(input);
		std::cout << prompt2 << std::endl;
	}
	return 0;
}


void testEvaluateExpression() {
	std::string input = "1+23";
	Expression expression(input);
	int actual = expression.evaluate();
	int expected = 24;
	assert(actual == expected);

	input = "1+23*(2+2)";
	expression = Expression(input);
	actual = expression.evaluate();
	expected = 93;
	assert(actual == expected);

	input = "1+2-3+4-5+6-7+8";
	expression = Expression(input);
	actual = expression.evaluate();
	expected = 6;
	assert(actual == expected);

	input = "(1+2-3)*4-5+(6-7)	*8";
	expression = Expression(input);
	actual = expression.evaluate();
	expected = -13;
	assert(actual == expected);

	input = "2*((1+2-3)*4-5+(6-7)	*8)";
	expression = Expression(input);
	actual = expression.evaluate();
	expected = -26;
	assert(actual == expected);

	input = "(2*((1+2-3)*4-5+(6-7)	*8))/(2-2)";
	expression = Expression(input);
	actual = expression.evaluate();
	assert(!expression.isValid());

	input = "(2*((1+2-3)*4-5+(6-7)	*8))/(2-4)";
	expression = Expression(input);
	actual = expression.evaluate();
	expected = 13;
	assert(actual == expected);

	input = "(((1+2-4)*4-5+(6-7)	*8))/(2-4)";
	expression = Expression(input);
	expected = 8;
	actual = expression.evaluate();
	assert(actual == expected);

	input = "(	2 * ( 3 * ( 4 * ( 5 + 6 ) + 7 ) + 8 ) - 10 ) * 2";
	expression = Expression(input);
	expected = 624;
	actual = expression.evaluate();
	assert(actual == expected);

	input = " 3 *	(-2*9)+6*(8+2)";
	expression = Expression(input);
	expected = 6;
	actual = expression.evaluate();
	assert(actual == expected);

	input = " 5   ";
	expression = Expression(input);
	expected = 5;
	actual = expression.evaluate();
	assert(actual == expected);


	input = "(3 - 1) * 2 + 3";
	expression = Expression(input);
	expected = 7;
	actual = expression.evaluate();
	assert(actual == expected);


}

void testExpressionValidate() {
	std::string input = "( 4 +1)-2   +45 * 2     -1 +2+3";
	Expression expression(input);
	assert(expression.isValid());

	input = "(4+(2-(1*2)*3))";
	expression = Expression(input);
	assert(expression.isValid());

	input = "( +(2-(1*2)*3))";
	expression = Expression(input);
	assert(expression.isValid());

	input = "( (1)+(2-(1*2)*3))";
	expression = Expression(input);
	assert(expression.isValid());

	input = " 1 ";
	expression = Expression(input);
	assert(expression.isValid());

	input = " (1 ";
	expression = Expression(input);
	assert(!expression.isValid());

	input = " 1 )";
	expression = Expression(input);
	assert(!expression.isValid());

	input = " 1 (2)";
	expression = Expression(input);
	assert(!expression.isValid());

	input = " 1 *(2)";
	expression = Expression(input);
	assert(expression.isValid());

	input = " 1 *	(2*9)+6(8+2)";
	expression = Expression(input);
	assert(!expression.isValid());

	input = " 1 *	(2*9)+6*(8+2)";
	expression = Expression(input);
	assert(expression.isValid());

	input = " 1 *	(-2*9)+6*(8+2)";
	expression = Expression(input);
	assert(expression.isValid());

	input = " 1 *	(-2*9)++6*(8+2)";
	expression = Expression(input);
	assert(!expression.isValid());

	input = "- 2";
	expression = Expression(input);
	assert(expression.isValid());
}

void testTokenStream() {
	std::string tokens = "12+0013-(22+6)*7";
	TokenStream ts = TokenStream(tokens);
	Token t = ts.nextToken();

	assert(t.getType() == '1');
	assert(t.getValue() == 12);
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '+');
	assert(t.getValue() == (int) '+');
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '0');
	assert(t.getValue() == 13);
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '-');
	assert(t.getValue() == (int) '-');
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '(');
	assert(t.getValue() == (int) '(');
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '2');
	assert(t.getValue() == 22);
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '+');
	assert(t.getValue() == (int) '+');
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '6');
	assert(t.getValue() == 6);
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == ')');
	assert(t.getValue() == (int) ')');
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '*');
	assert(t.getValue() == (int) '*');
	assert(ts.hasNextToken());

	t = ts.nextToken();
	assert(t.getType() == '7');
	assert(t.getValue() == 7);
	assert(!ts.hasNextToken());

	t = ts.nextToken();
	assert(t == AEUtils::dummyToken);
	assert(!ts.hasNextToken());

}

void testToken() {
	char x = '0';
	Token t(x);
	int actual = t.getValue();
	int expected = (int)x;
	assert(actual == expected);
	int val = 25;
	t = Token(x, val);

	actual = t.getValue();
	expected = 25;
	assert(actual == expected);

	x = '(';
	t = Token(x);
	char actChar = (char)t.getValue();
	char expChar = '(';
	assert(actChar == expChar);

	x = ')';
	t = Token(x);
	actChar = (char)t.getValue();
	expChar = ')';
	assert(actChar == expChar);

	x = '+';
	t = Token(x);
	actChar = (char)t.getValue();
	expChar = '+';
	assert(actChar == expChar);

	x = '-';
	t = Token(x);
	actChar = (char)t.getValue();
	expChar = '-';
	assert(actChar == expChar);

	x = '*';
	t = Token(x);
	actChar = (char)t.getValue();
	expChar = '*';
	assert(actChar == expChar);

	x = '/';
	t = Token(x);
	actChar = (char)t.getValue();
	expChar = '/';
	assert(actChar == expChar);

}

void testCheckBraceNesting() {
	std::string input = "(abc)";
	bool actual = checkBraceNesting(input, '(', ')');
	bool expected = true;
	assert(actual == expected);
	input = "((isui )	uh	)";
	actual = checkBraceNesting(input, '(', ')');
	expected = true;
	assert(actual == expected);

	input = "((isui )	)uh	)";
	actual = checkBraceNesting(input, '(', ')');
	expected = false;
	assert(actual == expected);

	input = "isui 	uh	";
	actual = checkBraceNesting(input, '(', ')');
	expected = true;
	assert(actual == expected);
}

void testValidate() {
	ExpressionValidator validator;
	std::string input = "(4+5)";
	std::string tokens;
	bool actual = validator.validate(input);
	bool expected = true;
	assert(actual == expected);

	input = "()";
	tokens.clear();
	actual = validator.validate(input);
	expected = false;
	assert(actual == expected);

	input = "((3+5))";
	tokens.clear();
	actual = validator.validate(input);
	expected = true;
	assert(actual == expected);

	input = "((3+))";
	tokens.clear();
	actual = validator.validate(input);
	expected = false;
	assert(actual == expected);

	input = "((+4))";
	tokens.clear();
	actual = validator.validate(input);
	expected = true;
	assert(actual == expected);

	input = "((5+4*(7+9)))";
	tokens.clear();
	actual = validator.validate(input);
	expected = true;
	assert(actual == expected);

	input = "((5+4*(7+9-)))";
	tokens.clear();
	actual = validator.validate(input);
	expected = false;
	assert(actual == expected);

	input = "((5+4*(7+9-(1))))";
	tokens.clear();
	actual = validator.validate(input);
	expected = true;
	assert(actual == expected);

	input = "((5+4*(7+9-(1)())))";
	tokens.clear();
	actual = validator.validate(input);
	expected = false;
	assert(actual == expected);

	input = "((5+4*(7+9(1+2))))";
	tokens.clear();
	actual = validator.validate(input);
	expected = false;
	assert(actual == expected);
}
