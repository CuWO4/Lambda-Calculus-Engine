#include "lambda.h"

#include <ctime>
#include <functional>

namespace lambda {

  template <typename T>
  static auto operator+(
    std::unordered_set<T>& a,
    std::unordered_set<T>& b
  ) -> std::unordered_set<T> {
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


  auto Expression::get_free_variables() -> std::unordered_set<std::string>& {
    return free_variables;
  }

  bool Expression::get_computational_priority() {
    return computational_priority_flag;
  }

  void Expression::set_computational_priority(bool computational_priority) {
    computational_priority_flag = computational_priority;
  }


  Variable::Variable(std::string literal, bool computational_priority): literal(literal) {
    free_variables = { literal };
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
    std::unordered_map<std::string, Expression*>& symbol_table,
    std::unordered_multiset<std::string>&& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (!has(bound_variables, literal) && has(symbol_table, literal)) {
      auto new_expr = symbol_table.find(literal)->second->clone(this->computational_priority_flag);
      delete this;
      return { new_expr, ReduceType::Delta };
    }

    computational_priority_flag = false;
    return { this, ReduceType::Null };
  }

  auto Variable::replace(
    Variable& variable,
    Expression& expression,
    std::unordered_multiset<std::string>&& bound_variables
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
    std::unordered_multiset<std::string>&& bound_variables
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
    bool new_computational_priority
  ) -> Expression* {
    return new Variable(
      literal,
      new_computational_priority
    );
  }

  bool Variable::is_computational_priority(
    std::unordered_multiset<std::string>&& bound_variables
  ) {
    return !has(bound_variables, literal) && computational_priority_flag;
  }


  Abstraction::Abstraction(
    Variable binder,
    Expression* body,
    bool computational_priority
  ): binder(binder), body(body) {
    free_variables = this->body->get_free_variables();
    free_variables.erase(this->binder.get_literal());
    computational_priority_flag = computational_priority;
  }

  Abstraction* Abstraction::get_instance(
    Variable binder,
    Expression *body,
    bool computational_priority
  ) {
    return new Abstraction(binder, body, computational_priority);
  }

  void Abstraction::delete_instance() {
    body->delete_instance();
    delete this;
  }

  auto Abstraction::alpha_reduce(
    Variable to
  ) -> Abstraction* {
    auto new_expr = Abstraction::get_instance(
      to,
      body->replace(binder, to, {} /* do not care and should not care */).first,
      computational_priority_flag
    );
    delete this;
    return new_expr;
  }

  auto Abstraction::reduce(
    std::unordered_map<std::string, Expression*>& symbol_table,
    std::unordered_multiset<std::string>&& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    bound_variables.emplace(binder.get_literal());
    auto [new_body, reduce_type] = body->reduce(
      symbol_table,
      std::move(bound_variables)
    );
    bound_variables.erase(bound_variables.find(binder.get_literal()));

    auto new_expr = new Abstraction(
      binder,
      new_body,
      (bool)reduce_type && computational_priority_flag
    );
    delete this;
    return { new_expr, reduce_type };
  }

  auto Abstraction::replace(
    Variable& variable,
    Expression& expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (variable == binder) {
      return { this, ReduceType::Null };
    }

    if (has(expression.get_free_variables(), binder.get_literal())) {
      for (int i = 0;; i++) {
        std::string new_literal = index_to_string(i);
        if (!has(bound_variables, new_literal)) {
          return this->alpha_reduce(
            Variable(new_literal)
          )->replace(
            variable,
            expression,
            std::move(bound_variables)
          );
        }
      }
    }

    bound_variables.emplace(binder.get_literal());
    auto [new_body, reduce_type] = body->replace(
      variable,
      expression,
      std::move(bound_variables)
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
    std::unordered_multiset<std::string>&& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    bound_variables.emplace(binder.get_literal());
    auto result = body->replace(
      binder,
      expression,
      std::move(bound_variables)
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
    bool new_computational_priority
  ) -> Expression* {
    return new Abstraction(
      binder,
      body->clone(),
      new_computational_priority
    );
  }

  bool Abstraction::is_computational_priority(
    std::unordered_multiset<std::string>&& bound_variables
  ) {
    for(auto free_variable: free_variables) {
      if (has(bound_variables, free_variable)) { return false; }
    }
    return computational_priority_flag;
  }


  Application::Application(
    Expression* first,
    Expression* second,
    bool computational_priority
  ): first(first), second(second) {
    free_variables = this->first->get_free_variables()
      + this->second->get_free_variables();
    computational_priority_flag = computational_priority;
  }

  Application* Application::get_instance(
    Expression* first,
    Expression* second,
    bool computational_priority
  ) {
    return new Application(first, second, computational_priority);
  }

  void Application::delete_instance() {
    first->delete_instance();
    second->delete_instance();
    delete this;
  }

  auto Application::reduce_first(
    std::unordered_map<std::string, Expression*>& symbol_table,
    std::unordered_multiset<std::string>& bound_variables
  ) -> std::pair<bool, ReduceType> {
    auto pair = first->reduce(symbol_table, std::move(bound_variables));
    first = pair.first;
    auto reduce_type = pair.second;

    computational_priority_flag = (bool)reduce_type && computational_priority_flag;
    return { (bool)reduce_type, reduce_type };
  }

  auto Application::reduce_second(
    std::unordered_map<std::string, Expression*>& symbol_table,
    std::unordered_multiset<std::string>& bound_variables
  ) -> std::pair<bool, ReduceType> {
    auto pair = second->reduce(symbol_table, std::move(bound_variables));
    second = pair.first;
    auto reduce_type = pair.second;

    computational_priority_flag = (bool)reduce_type && computational_priority_flag;
    return { (bool)reduce_type, reduce_type };
  }

  auto Application::reduce(
    std::unordered_map<std::string, Expression*>& symbol_table,
    std::unordered_multiset<std::string>&& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (first->is_computational_priority(std::move(bound_variables))) {
      auto pair = reduce_first(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };
    }
    else if (second->is_computational_priority(std::move(bound_variables))) {
      auto pair = reduce_second(symbol_table, bound_variables);
      if (pair.first) return { this, pair.second };
    }
    
    auto [new_expr, reduce_type] = first->apply(*second, std::move(bound_variables));
    if ((bool)reduce_type) {
      new_expr->set_computational_priority(computational_priority_flag);
      delete this;
      return {new_expr, reduce_type};
    }
    
    auto pair = reduce_first(symbol_table, bound_variables);
    if (pair.first) return { this, pair.second };

    pair = reduce_second(symbol_table, bound_variables);
    if (pair.first) return { this, pair.second };

    return { this, ReduceType::Null };
  }

  auto Application::replace(
    Variable& variable,
    Expression& expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    auto [new_first, first_reduce_type] = first->replace(
      variable,
      expression,
      std::move(bound_variables)
    );
    first = new_first;

    auto [new_second, second_reduce_type] = second->replace(
      variable,
      expression,
      std::move(bound_variables)
    );
    second = new_second;

    return {
      this,
      (bool)first_reduce_type
        ? first_reduce_type
        : second_reduce_type
    };
  }

  auto Application::apply(
    Expression& expression,
    std::unordered_multiset<std::string>&& bound_variables
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
    bool new_computational_priority
  ) -> Expression* {
    return new Application(
      first->clone(),
      second->clone(),
      new_computational_priority
    );
  }

  bool Application::is_computational_priority(
    std::unordered_multiset<std::string>&& bound_variables
  ) {
    return computational_priority_flag
      || first->is_computational_priority(std::move(bound_variables))
      || second->is_computational_priority(std::move(bound_variables))
    ;
  }


  static auto generate_church_number_body(unsigned number) -> Expression* {
    if (number == 0) { return new Variable("x"); }

    return Application::get_instance(
      new Variable("f"),
      generate_church_number_body(number - 1)
    );
  }

  auto generate_church_number(unsigned number) -> Expression* {
    return Abstraction::get_instance(
      Variable("f"),
      Abstraction::get_instance(
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

  clock_t msec_count(std::function<void(void)> func) {
    auto start_time = clock();
    func();
    auto end_time = clock();
    return (end_time - start_time) / (double)CLOCKS_PER_SEC * 1000;
  }

  auto Reducer::reduce(
     Expression* expression
  ) -> std::pair<std::string, Expression*> {

    auto result_string = expression->to_string() + "\n";
    auto expr = expression->clone();

    unsigned long long step;

    auto msec = msec_count([&]() {
      for (step = 0;; step++) {
        auto pair = expr->reduce(symbol_table, {});
        expr = pair.first;
        auto reduce_type = pair.second;

        if (reduce_type == ReduceType::Null) { break; }

        result_string += reduce_type_to_header(reduce_type) + expr->to_string() + "\n";
      }
    });

    result_string += "\nto be sought:    " + expression->to_string() + "\n"
                  +    "result:          " + expr->to_string() + "\n"
                  +    "step taken:      " + std::to_string(step) + "\n"
                  +    "character count: " + std::to_string(result_string.length()) + "\n"
                  +    "time cost:       " + std::to_string(msec) + "ms" +"\n";

    return {result_string, expr};
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