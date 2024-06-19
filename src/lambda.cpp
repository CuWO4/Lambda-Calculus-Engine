#include "lambda.h"

#include <ctime>
#include <functional>

namespace lambda {

  template <typename T>
  static auto operator+(
    std::set<T>& a,
    std::set<T>& b
  ) -> std::set<T> {
    auto result = a;
    result.insert(b.begin(), b.end());
    return result;
  }

  template <template<typename...> typename Container, typename T, typename... Ts>
  static bool has(Container<T, Ts...>& container, T& element) {
    return container.count(element) > 0;
  }

  static auto index_to_string(unsigned index) -> std::string {
    constexpr unsigned LETTER_N = 26;
    auto result = std::string("");

    for (; index >= LETTER_N; index /= LETTER_N) {
      result += 'a' + index % LETTER_N;
    }
    result += 'a' + index % LETTER_N;

    return result;
  }

  static bool is_number(std::string s) {
    for (auto ch: s) {
      if (ch < '0' || ch > '9') { return false; }
    }
    return true;
  }



  ComputationalPriority remove_lazy(ComputationalPriority computational_priority) {
    return computational_priority == ComputationalPriority::Lazy
      ? ComputationalPriority::Neutral
      : computational_priority
    ;
  }


  auto Expression::get_free_variables() -> std::set<std::string>& {
    update_free_variables();
    return free_variables;
  }

  void Expression::set_computational_priority(
    ComputationalPriority computational_priority
  ) {
    computational_priority_flag = computational_priority;
  }


  Variable::Variable(
    std::string literal, 
    ComputationalPriority computational_priority
  ) : literal(literal) {
    computational_priority_flag = computational_priority;
  }

  void Variable::delete_instance() {
    delete this;
  }

  bool Variable::operator==(Variable& right) {
    return literal == right.literal;
  }
  bool Variable::operator==(Variable&& right) {
    return literal == right.literal;
  }

  auto Variable::get_literal() -> std::string& { return literal; }

  auto Variable::reduce(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    computational_priority_flag = remove_lazy(computational_priority_flag);

    if (is_number(literal)) {
      auto new_expr = generate_church_number(atoi(literal.c_str()));
      new_expr->set_computational_priority(computational_priority_flag);
      delete this;
      return { new_expr, ReduceType::Delta };
    }

    if (!has(bound_variables, literal) && has(symbol_table, literal)) {
      auto new_expr = symbol_table.find(literal)->second->clone(computational_priority_flag);
      delete this;
      return { new_expr, ReduceType::Delta };
    }

    computational_priority_flag = ComputationalPriority::Neutral;
    return { this, ReduceType::Null };
  }

  auto Variable::replace(
    Variable& variable,
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (*this == variable) {
      auto new_expr = expression.clone(computational_priority_flag);
      delete this;
      return { new_expr, ReduceType::Beta };
    }

    return { this, ReduceType::Null };
  }

  auto Variable::apply(
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    return { this, ReduceType::Null };
  }

  auto Variable::to_string() -> std::string {
    return literal;
  }

  auto Variable::get_priority() -> Priority {
    return Priority::Variable;
  }

  auto Variable::clone() -> Expression* {
    return new Variable(
      literal,
      computational_priority_flag
    );
  }
  auto Variable::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    return new Variable(
      literal,
      new_computational_priority
    );
  }
  
  bool Variable::is_eager(
    std::multiset<std::string>& bound_variables
  ) {
    return 
      computational_priority_flag == ComputationalPriority::Eager
      && !has(bound_variables, literal) 
      && !is_number(literal)
    ;
  }

  bool Variable::is_lazy() {
    return computational_priority_flag == ComputationalPriority::Lazy;
  }

  void Variable::update_free_variables() {
    if (is_free_variables_updated) { return; }

    free_variables = { literal };
      
    is_free_variables_updated = true;
  }


  Abstraction::Abstraction(
    Variable binder,
    Expression* body,
    ComputationalPriority computational_priority
  ): binder(binder), body(body) {
    computational_priority_flag = computational_priority;
  }

  void Abstraction::delete_instance() {
    body->delete_instance();
    delete this;
  }

  auto Abstraction::alpha_reduce(
    Variable to
  ) -> Abstraction* {
    // empty, alpha reduce do not care and should not care bound variables
    std::multiset<std::string> bound_variables;
    auto new_expr = new Abstraction(
      to,
      body->replace(binder, to, bound_variables).first,
      computational_priority_flag
    );
    delete this;
    return new_expr;
  }

  auto Abstraction::reduce(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    computational_priority_flag = remove_lazy(computational_priority_flag);

    bound_variables.emplace(binder.get_literal());
    auto [new_body, reduce_type] = body->reduce(
      symbol_table,
      bound_variables
    );
    bound_variables.erase(bound_variables.find(binder.get_literal()));

    auto new_expr = new Abstraction(
      binder,
      new_body,
      (bool)reduce_type 
        ? computational_priority_flag
        : ComputationalPriority::Neutral
    );
    delete this;
    return { new_expr, reduce_type };
  }

  auto Abstraction::replace(
    Variable& variable,
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (variable == binder) {
      return { this, ReduceType::Null };
    }

    if (has(expression.get_free_variables(), binder.get_literal())) {
      for (int i = 0;; i++) {
        std::string new_literal = index_to_string(i);
        if (
          !has(bound_variables, new_literal) 
          && !has(free_variables, new_literal)
          && new_literal != binder.get_literal()
        ) {
          return this->alpha_reduce(
            Variable(new_literal)
          )->replace(
            variable,
            expression,
            bound_variables
          );
        }
      }
    }

    bound_variables.emplace(binder.get_literal());
    auto [new_body, reduce_type] = body->replace(
      variable,
      expression,
      bound_variables
    );
    bound_variables.erase(bound_variables.find(binder.get_literal()));

    auto new_expr = new Abstraction(
      binder,
      new_body,
      computational_priority_flag
    );
    delete this;
    return { new_expr, reduce_type };
  }

  auto Abstraction::apply(
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    bound_variables.emplace(binder.get_literal());
    auto result = body->replace(
      binder,
      expression,
      bound_variables
    ).first;
    result->set_computational_priority(computational_priority_flag);
    bound_variables.erase(bound_variables.find(binder.get_literal()));

    delete this;
    return { result, ReduceType::Beta };
  }

  auto Abstraction::to_string() -> std::string {
    return
      "\\" + binder.to_string()
      + "." + (body->get_priority() > Priority::Abstraction ? " " : "")
      + body->to_string()
    ;
  }

  auto Abstraction::get_priority() -> Priority {
    return Priority::Abstraction;
  }

  auto Abstraction::clone() -> Expression* {
    return new Abstraction(
      binder,
      body->clone(),
      computational_priority_flag
    );
  }
  auto Abstraction::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    return new Abstraction(
      binder,
      body->clone(),
      new_computational_priority
    );
  }

  bool Abstraction::is_eager(
    std::multiset<std::string>& bound_variables
  ) {
    if (is_lazy()) return false;
    for(auto free_variable: free_variables) {
      if (has(bound_variables, free_variable)) { return false; }
    }
    return computational_priority_flag == ComputationalPriority::Eager;
  }

  bool Abstraction::is_lazy() {
    return computational_priority_flag == ComputationalPriority::Lazy;
  }

  void Abstraction::update_free_variables() {
    if (is_free_variables_updated) { return; }

    free_variables = this->body->get_free_variables();
    free_variables.erase(this->binder.get_literal());
      
    is_free_variables_updated = true;
  }



  Application::Application(
    Expression* first,
    Expression* second,
    ComputationalPriority computational_priority
  ): first(first), second(second) {
    computational_priority_flag = computational_priority;
  }

  void Application::delete_instance() {
    first->delete_instance();
    second->delete_instance();
    delete this;
  }

  auto Application::reduce_first(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<bool, ReduceType> {
    auto pair = first->reduce(symbol_table, bound_variables);
    first = pair.first;
    auto reduce_type = pair.second;

    computational_priority_flag = (bool)reduce_type 
      ? computational_priority_flag
      : ComputationalPriority::Neutral;
    is_free_variables_updated = false;

    return { (bool)reduce_type, reduce_type };
  }

  auto Application::reduce_second(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<bool, ReduceType> {
    auto pair = second->reduce(symbol_table, bound_variables);
    second = pair.first;
    auto reduce_type = pair.second;

    computational_priority_flag = (bool)reduce_type 
      ? computational_priority_flag
      : ComputationalPriority::Neutral;
    is_free_variables_updated = false;

    return { (bool)reduce_type, reduce_type };
  }

  auto Application::reduce(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    computational_priority_flag = remove_lazy(computational_priority_flag);

    if (first->is_eager(bound_variables)) {
      auto pair = reduce_first(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };
    }
    if (second->is_eager(bound_variables)) {
      auto pair = reduce_second(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };
    }

    auto [new_expr, reduce_type] = first->apply(*second, bound_variables);
    if ((bool)reduce_type) {
      new_expr->set_computational_priority(computational_priority_flag);
      delete this;
      return {new_expr, reduce_type};
    }

    if (first->is_lazy()) {
      auto pair = reduce_second(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };

      pair = reduce_first(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };
    }
    else {
      auto pair = reduce_first(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };

      pair = reduce_second(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };
    }

    return { this, ReduceType::Null };
  }

  auto Application::replace(
    Variable& variable,
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    ReduceType first_reduce_type, second_reduce_type;

    std::tie(first, first_reduce_type) = first->replace(
      variable,
      expression,
      bound_variables
    );

    std::tie(second, second_reduce_type) = second->replace(
      variable,
      expression,
      bound_variables
    );

    return {
      this,
      (bool)first_reduce_type
        ? first_reduce_type
        : second_reduce_type
    };
  }

  auto Application::apply(
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    return { this, ReduceType::Null };
  }

  auto Application::to_string() -> std::string {
    auto result = std::string("");

    if (first->get_priority() < get_priority()) {
      result += "(" + first->to_string() + ")";
    }
    else { result += first->to_string(); }

    result += " ";

    if (second->get_priority() <= get_priority()) {
      result += "(" + second->to_string() + ")";
    }
    else { result += second->to_string(); }

    return result;
  }

  auto Application::get_priority() -> Priority {
    return Priority::Application;
  }

  auto Application::clone() -> Expression* {
    return new Application(
      first->clone(),
      second->clone(),
      computational_priority_flag
    );
  }
  auto Application::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    return new Application(
      first->clone(),
      second->clone(),
      new_computational_priority
    );
  }

  bool Application::is_eager(
    std::multiset<std::string>& bound_variables
  ) {
    if (is_lazy()) return false;
    return computational_priority_flag == ComputationalPriority::Eager
      || first->is_eager(bound_variables)
      || second->is_eager(bound_variables)
    ;
  }

  bool Application::is_lazy() {
    return computational_priority_flag == ComputationalPriority::Lazy;
  }

  void Application::update_free_variables() {
    if (is_free_variables_updated) { return; }

    free_variables = this->first->get_free_variables()
      + this->second->get_free_variables();

    is_free_variables_updated = true;
  }


  static auto generate_church_number_body(unsigned number) -> Expression* {
    if (number == 0) { return new Variable("x"); }

    return new Application(
      new Variable("f"),
      generate_church_number_body(number - 1)
    );
  }

  auto generate_church_number(unsigned number) -> Expression* {
    return new Abstraction(
      Variable("f"),
      new Abstraction(
        Variable("x"),
        generate_church_number_body(number)
      )
    );
  }

  static std::string reduce_type_to_header(ReduceType reduce_type) {
    if (reduce_type == ReduceType::Alpha) { return "alpha> "; }
    else if (reduce_type == ReduceType::Beta) { return "beta>  "; }
    else if (reduce_type == ReduceType::Delta) { return "delta> "; }
    else /* reduce_type == ReduceType::NoReduce */ { return ""; }
  }

  static void string_println(std::string& s, FILE* out) {
    fprintf(out, "%s\n", s.c_str());
  }

  static void string_println(std::string&& s, FILE* out) {
    fprintf(out, "%s\n", s.c_str());
  }

  clock_t msec_count(std::function<void(void)> func) {
    auto start_time = clock();
    func();
    auto end_time = clock();
    return (end_time - start_time) / (double)CLOCKS_PER_SEC * 1000;
  }

  auto Reducer::reduce(
     Expression* expression, FILE* out, bool display_process
  ) -> Expression* {

    string_println(expression->to_string(), out);
    fprintf(out, "\n");
    
    auto expr = expression->clone();

    unsigned long long step;
    unsigned long long character_count = 0;

    auto msec = msec_count([&]() {
      for (step = 0;; step++) {
        std::multiset<std::string> bound_variables;
        auto pair = expr->reduce(symbol_table, bound_variables);
        expr = pair.first;
        auto reduce_type = pair.second;

        if (reduce_type == ReduceType::Null) { break; }

        if (display_process) {
          auto&& str = reduce_type_to_header(reduce_type) + expr->to_string();
          character_count += str.length();
          string_println(str, out);
        }
      }
    });

    fprintf(out, "\n");
    string_println("to be sought:     " + expression->to_string(), out);
    string_println("result:           " + expr->to_string(), out);
    string_println("step taken:       " + std::to_string(step), out);
    if (display_process) {
      string_println("character count:  " + std::to_string(character_count), out);
    }
    string_println("time cost:        " + std::to_string(msec) + "ms", out);
    fprintf(out, "\n");

    return expr;
  }


  void Reducer::register_symbol(
    std::string literal,
    Expression* expression
  ) {
    symbol_table[literal] = expression;
  }

  Reducer::~Reducer() {
    for (auto symbol: symbol_table) {
      symbol.second->delete_instance();
    }
  }

}