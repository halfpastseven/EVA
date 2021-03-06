#include "circuitelement.h"
#include <QChar>
#include <QtXml/QDomDocument>

QVector <token> parseInput  (QString input);
QVector <token> shuntingYard(QVector<token> input);

inline QVector<token> parse (QString input) {return shuntingYard(parseInput(input));}


inline std::complex<double> myTanh(std::complex<double> x){
    return (x.real() > 0) ? (1.-std::exp(-x*2.)) / (1.+std::exp(-x*2.)) 
                          : (std::exp(x*2.)-1.) / (std::exp(x*2.)-1.);
}

inline double myTanh (double x) { return (x>0) ? (1-exp(-2*x))/(1+exp(-2*x))
                                               : (exp(2*x )-1)/(exp(2*x)+1);}

inline QVector <token> errorMsg (QString Msg) {
    token Error (token::Null);
    Error.s = Msg;
    return QVector <token>(1,Msg);
}

inline bool isNumeric (QChar c) {return c.isDigit() || c == ".";}
inline bool isSymbol  (QChar c) {return c.isLetter();}
inline bool isOperator(QChar c) {
    return c==QChar('+')||c==QChar('-')||c==QChar('*')||c==QChar('/')|| c==QChar('^');
}
inline bool isBracket (QChar c) {return c==QChar('(')||c==QChar(')');}
inline bool isInvalid (QChar c) {return !(isNumeric(c)|| isSymbol(c)||isOperator(c)||isBracket(c));}

CircuitElement::CircuitElement(QFile *fromXML):
    importErrorMsg()
{
    QDomDocument domDoc;
    int importErrorLine;
    ImportSuccess = 0;
    bool readFileSuccess=domDoc.setContent(fromXML,&importErrorMsg, &importErrorLine);
    
    if (!readFileSuccess) {
        importErrorMsg = importErrorMsg + QString("(line: %1)").arg(importErrorLine);
        return;
    }
    QDomElement rootElem = domDoc.firstChildElement("element");
    if (rootElem.isNull()){
        importErrorMsg = "Top element must be <element>";
        return;
    }
    
    QString elemName = rootElem.firstChildElement("name").text();
    if (elemName.isEmpty()) {
        importErrorMsg = "Element name not found";
        return;
    }
    ElemType = elemName;
    
    QString elemSymbol = rootElem.firstChildElement("symbol").text();
    if (elemSymbol.isEmpty()){
        importErrorMsg = "Element symbol not found";
        return;
    }
    Symbol = elemSymbol;
    
    QString elemExprText = rootElem.firstChildElement("expression").text();
    // Save the expression for parsing later
    if (elemExprText.isEmpty()){
        importErrorMsg = "Element expression not found";
        return;
    }     
    
    QDomElement elemPar = rootElem.firstChildElement("parameter");
    if (elemPar.text().isEmpty()){
        importErrorMsg = "Parameter definition not found";
        return;
    }

    for (; !elemPar.isNull(); elemPar = elemPar.nextSiblingElement("parameter")) {
        bool parseOK;
           
        QString parName = elemPar.firstChildElement("variable").text();
        if (parName.isEmpty()) {
            importErrorMsg = QString("Variable name missing for parameter #%1")
                    .arg(VarNames.size()+1);
            return;
        }
        VarNames.push_back(parName);
 
        double parVal = elemPar.firstChildElement("value").text().toDouble(&parseOK);
        if (!parseOK) {
            importErrorMsg = QString("Parameter value incorrect for \'%1\'")
                    .arg(parName);
            return;
        }
        VarVal.push_back(parVal);
        
        double parMin = elemPar.firstChildElement("minimum").text().toDouble(&parseOK);
        if (!parseOK) {
            importErrorMsg = QString("Parameter minimum incorrect for \'%1\'")
                    .arg(parName);
            return;
        }
        VarMin.push_back(parMin);
        
        double parMax = elemPar.firstChildElement("maximum").text().toDouble(&parseOK);
        if (!parseOK) {
            importErrorMsg = QString("Parameter maximum incorrect for \'%1\'")
                    .arg(parName);
            return;
        }
        VarMax.push_back(parMax);
        
        VarFixed.push_back(0);
    }
    
    Description = rootElem.firstChildElement("description").text();
    
    IconName = rootElem.firstChildElement("icon").text();
            
    QDomElement elemSource = rootElem.firstChildElement("source");
    if (!elemSource.isNull()){
        Ref = elemSource.firstChildElement("reference").text();
        Doi = elemSource.firstChildElement("doi").text();
    }
    
    formula = parse(elemExprText);
    if (!formula.isEmpty() && formula[0].s.startsWith("Error")) {
        importErrorMsg=formula[0].s;
        return;
    }
    
    /// Validate the parsed expression against the list of variables
    QVector <bool> matchFormulaVarlist (VarNames.size(),0);

    for (int i=0; i<formula.size(); i++){
        if (formula[i].Type==token::Variable && 
                formula[i].s != QString("f") &&
                formula[i].s != QString("w") &&
                formula[i].s != QString("i") &&
                formula[i].s != QString("pi")
                ){
            int foundMatch = VarNames.indexOf(formula[i].s);
            if (foundMatch == -1) {
                importErrorMsg= QString("Parameter \'%1\' in the expression not found in the list")
                        .arg(formula[i].s);
                return;
            }
            else matchFormulaVarlist[foundMatch] = 1;
        }
    }
    if (matchFormulaVarlist.contains(0)) {
        importErrorMsg= QString("Parameter \'%1\' in the list not used in the expression")
                .arg(VarNames.at(matchFormulaVarlist.indexOf(0)));
        return;
    }
        
    ImportSuccess = 1;
}

QString CircuitElement::toRPN() const
{
    QString Output;
    for (int i =0; i<formula.size(); i++){
        const token &t = formula.at(i);
        QString tks; // Token string
        switch (t.Type){
        case token::Null:
            return t.s; break;
        case token::Numeric:
            tks = QString::number(t.v); break;
        case token::Complex:
            tks = QString("(%1+%2i)").arg(t.z.real()).arg(t.z.imag()); break;
        case token::Variable:
            tks = t.s; break;
        case token::Function:
            tks = QString("[") + token::stdFun[t.f]+ "]"; break;
        case token::Operator:
        {
            switch (t.o){
            case token::OpAdd: tks = "+"; break;
            case token::OpMin: tks = "-"; break;
            case token::OpMul: tks = "*"; break;
            case token::OpDiv: tks = "/"; break;
            case token::OpPow: tks = "^"; break;
            default: break;
            }
        }
            break;        
        }
        Output.append(tks);
        Output.append(" ");
    }
    
    return Output;
}

QVector<token> parseInput(QString input){

    QVector <token> sortedInput;
	
    if (!input.size()) {
		return errorMsg("Error: empty input");
	}
	
	// Remove blanks
	int pos = 0;
	while (pos<input.size()) {
		if (input[pos]==' '||input[pos]==0x0d) {
			input.remove(pos, 1);
			continue;
		}
		pos++;
	};
	
	// Preparation work
	pos = 0;
	
	while (pos<input.size()){
        //char c = input[pos];
        
		if (isInvalid (input[pos])) {
			return errorMsg(QString("Error: invalid character at [%1]: [%2]")
                            .arg(pos).arg(input[pos]));
		}
		
		if (isNumeric(input[pos])){  /// ...........................NUMERIC
			QString Numeric;
            
            int DECflag = 0; // Will only allow one decimal point
            int SNflag=0;  // Storing scientific notation parsing state
                            // 0 - Not yet met
                            // 1 - 'e' or 'E' appeared
                            // 2 - '+' or '-' followed 'E' or 'e'
                            // 3 - Number followed 'E' or 'e'
            // SNState does not guarantee valid input. Must be examined further
			
            while (true){
                if (pos==input.size()) break;
                
                if (input[pos]=='.'){
                    if (DECflag) {
                        return errorMsg("Error: dual decimal dots");
                    }
                    if (SNflag>0){
                        return errorMsg("Error: decimal dot in Exp");
                    }
                    DECflag = 1;
                }
                else if (input [pos] == 'e'|| input[pos]=='E'){
                    if (SNflag>0) {
                        return errorMsg("Error: incorrect scientific format");
                    }
                    else {
                        SNflag = 1;
                    }
                }
                else if (input[pos] == '+' || input[pos] == '-'){
                    if (SNflag==1) {
                        SNflag = 2;
                    }
                    else {
                        break;
                    }
                }
                else if (isSymbol(input[pos])) {
                    return errorMsg(QString("Error: invalid character at [%1]: [%2]")
                                    .arg(pos).arg(input[pos]));
                }
                else if (isNumeric(input[pos])&& SNflag>0 ){
                    SNflag=3;
                }
                else if (isOperator(input[pos]) || isBracket(input[pos])){
                    break;
                }
                
				Numeric.push_back(input[pos]);
				pos++;
			}
			
			sortedInput.push_back (token(Numeric.toDouble()));
		}
		
		else if (isSymbol(input[pos])){ /// ...........................SYMBOL
			QString Symbol;
			while (pos<input.size() && 
                    (isSymbol(input[pos]) || isNumeric(input[pos]))){
				Symbol.push_back (input [pos]);
				pos++;
			}
            if (token::isStdFunc(Symbol)!=-1){
                sortedInput.push_back(token (token::isStdFunc(Symbol)));
            }
			else sortedInput.push_back(token (Symbol));
		}
		else { // Is operator or bracket
            switch (input[pos].toLatin1()){
            case '+': sortedInput.push_back((token(token::OpAdd,2))); break;
            case '-': sortedInput.push_back((token(token::OpMin,2))); break;
            case '*': sortedInput.push_back((token(token::OpMul,1))); break;
            case '/': sortedInput.push_back((token(token::OpDiv,1))); break;
            case '^': sortedInput.push_back((token(token::OpPow,0))); break;
            case '(': sortedInput.push_back((token(token::LBra, 0))); break;
            case ')': sortedInput.push_back((token(token::RBra, 0))); break;
            default: break;
            }
			pos++;
		}
	}
	return (sortedInput);
}

QVector <token> shuntingYard(QVector<token> input){
	if (input.size()&& input[0].s.startsWith(QString("Error"))) return input;
	
	QVector<token> operatorStack;
	QVector<token> output;
	
	for (int i =0; i< input.size(); i++){
        
        switch (input[i].Type){
        
        case token::Numeric:
        case token::Complex:
            output.push_back(input[i]); 
            break;

        case token::Variable:
            if (input[i].s==QString("i")) {
                output.push_back(token(std::complex<double>(0,1)));
            }
            else if (input[i].s==QString("pi")){
                output.push_back(token(token::Pi));
            }
            else output.push_back(input[i]);
            break;

        case token::Operator:
            if (input[i].o == token::LBra) operatorStack.push_back(input[i]);
            else if (input[i].o==token::RBra) {
                while (operatorStack.back().o!=token::LBra){
                    output.push_back(operatorStack.back());
                    operatorStack.pop_back();
                    if (!operatorStack.size()) {
                        return errorMsg("Error: unpaired right bracket :)"); 
                    }
                }
                operatorStack.pop_back();
            }
            else { // input[i] is an operator
                    /// If the incoming symbol is an operator and
                    /// has either 
                    ///     LOWER precedence than the operator on the top of the stack, or
                    ///     has 
                    ///         the same precedence as the operator on the top of the stack and
                    ///         is LEFT associative -- 
                    /// continue to pop the stack until this is not true. 
                while (operatorStack.size() && operatorStack.back().Type==token::Operator &&
                       operatorStack.back().o!=token::LBra && operatorStack.back().o!=token::RBra&&
                            (operatorStack.back().opPrec < input[i].opPrec || 
                                (operatorStack.back().opPrec == input[i].opPrec &&
                                (!input[i].rightAss)))){
                    output.push_back(operatorStack.back());
                    operatorStack.pop_back();
                }
                operatorStack.push_back(input[i]);
            }
            break;
            
        case token::Function:    
            operatorStack.push_back(input[i]);
            break;
        default:
            break;
        }
	}
    while (operatorStack.size()) {
        if (operatorStack.back().o == token::LBra){
            return errorMsg("Error: unpaired left bracket :(");
        }
        output.push_back(operatorStack.back());
        operatorStack.pop_back();
    }
    return output;

}


const char* const token::stdFun [] = {
    "sin", "cos", "tan", 
    "sinh", "cosh", "tanh","coth",
    "abs", "sqrt", 
    "ln", "log", "exp"
};


/** Reference to the shunting yard algorithm
 *  http://www.oxfordmathcenter.com/drupal7/node/628

Summary of the Rules

If the incoming symbols is an operand, print it..

If the incoming symbol is a left parenthesis, push it on the stack.

If the incoming symbol is a right parenthesis: discard the right parenthesis, 
pop and print the stack symbols until you see a left parenthesis. Pop the left 
parenthesis and discard it.

If the incoming symbol is an operator and the stack is empty or contains a left
parenthesis on top, push the incoming operator onto the stack.

If the incoming symbol is an operator and has either higher precedence than the 
operator on the top of the stack, or has the same precedence as the operator on 
the top of the stack and is right associative -- push it on the stack.

If the incoming symbol is an operator and has either lower precedence than the 
operator on the top of the stack, or has the same precedence as the operator on 
the top of the stack and is left associative -- continue to pop the stack until 
this is not true. Then, push the incoming operator.

At the end of the expression, pop and print all operators on the stack. (No 
parentheses should remain.)

*/
