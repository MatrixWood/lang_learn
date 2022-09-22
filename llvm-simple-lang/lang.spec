calc: （"with" ident ("," ident)* ":")? expr ;
expr: term (("+" | "-") term)* ;
term: factor (("*" | "/") factor)* ;
factor: ident | number | "(" expr ")" ;
ident: ([a-zA-Z])+ ;
number: ([0-9])+ ;