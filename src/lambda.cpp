#include "lambda.h"

#include <ctime>
#include <functional>
#include <cassert>

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
      [[likely]] if (ch < '0' || ch > '9') { return false; }
    }
    return true;
  }


  Expression::Expression(ComputationalPriority computational_priority)
    : computational_priority_flag(computational_priority),
      is_is_eager_flag_updated(false),
      is_eager_flag(false), 
      is_normal_form(false) {}

  void Expression::set_computational_priority(
    ComputationalPriority computational_priority
  ) {
    if (computational_priority != computational_priority_flag) {
      is_is_eager_flag_updated = false;
    }
    computational_priority_flag = computational_priority;
  }

  bool Expression::is_lazy() {
    return computational_priority_flag == ComputationalPriority::Lazy;
  }

  bool Expression::is_eager(
    std::multiset<std::string>& bound_variables
  ) {
    update_eager_flag(bound_variables);
    return is_eager_flag;
  }

  bool Expression::is_variable_free(std::string&& literal) {
    return is_variable_free(literal);
  }


  Root::Root(Expression* expression)
    : Expression(ComputationalPriority::Neutral),
      expression(expression) {}

  void Root::delete_instance() {
    expression->delete_instance();
    delete this;
  }

  auto Root::reduce(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    auto [new_expr, reduce_type] = expression->reduce(symbol_table, bound_variables);
    return { new Root(new_expr), reduce_type };
  }

  auto Root::replace(
    Variable& variable,
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    return { this, ReduceType::Null };
  }

  auto Root::apply(
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    return { this, ReduceType::Null };
  }

  auto Root::to_string() -> std::string {
    return expression->to_string();
  }

  auto Root::get_priority() -> Priority {
    return Priority::Abstraction;
  }

  auto Root::clone() -> Expression* {
    return this->clone(computational_priority_flag);
  }

  auto Root::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    auto result = new Root(expression->clone());

    result->is_normal_form = is_normal_form;
    
    result->is_is_eager_flag_updated = is_is_eager_flag_updated;
    result->is_eager_flag = is_eager_flag;

    result->set_computational_priority(new_computational_priority);

    return result;
  }

  bool Root::is_variable_free(const std::string& literal) {
    return expression->is_variable_free(literal);
  }


  void Root::update_eager_flag(
    std::multiset<std::string>& bound_variables
  ) {
    if (is_is_eager_flag_updated) { return; }
    is_is_eager_flag_updated = true;
  }

  Variable::Variable(
    std::string literal, 
    ComputationalPriority computational_priority
  ) : Expression(computational_priority), literal(literal) {}

  void Variable::delete_instance() {
    delete this;
  }

  bool Variable::operator==(Variable& right) {
    return literal == right.literal;
  }
  bool Variable::operator==(Variable&& right) {
    return literal == right.literal;
  }

  auto Variable::get_literal() -> const std::string& { return literal; }

  auto Variable::reduce(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (is_normal_form) { 
      return { this, ReduceType::Null }; 
    }

    if (computational_priority_flag == ComputationalPriority::Lazy) {
      set_computational_priority(ComputationalPriority::Neutral);
    }

    if (has(bound_variables, literal)) {
      is_normal_form = true;
      return { this, ReduceType::Null };
    }

    [[unlikely]] if (is_number(literal)) {
      auto new_expr = generate_church_number(atoi(literal.c_str()));
      new_expr->set_computational_priority(computational_priority_flag);
      delete this;
      return { new_expr, ReduceType::Delta };
    }

    [[unlikely]] if (has(symbol_table, literal)) {
      auto new_expr = symbol_table.find(literal)->second
        ->clone(computational_priority_flag);
      delete this;
      return { new_expr, ReduceType::Delta };
    }

    set_computational_priority(ComputationalPriority::Neutral);
    is_normal_form = true;
    return { this, ReduceType::Null };
  }

  auto Variable::replace(
    Variable& variable,
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    [[unlikely]] if (*this == variable) {
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
    return this->clone(computational_priority_flag);
  }
  auto Variable::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    auto result = new Variable(literal);

    result->is_normal_form = is_normal_form;

    result->is_is_eager_flag_updated = is_is_eager_flag_updated;
    result->is_eager_flag = is_eager_flag;

    result->set_computational_priority(new_computational_priority);

    return result;
  }

  bool Variable::is_variable_free(const std::string& literal) {
    return this->literal == literal;
  }
  
  void Variable::update_eager_flag(
    std::multiset<std::string>& bound_variables
  ) {
    if (is_is_eager_flag_updated) { return; }
    is_is_eager_flag_updated = true;

    is_eager_flag = 
      !is_normal_form
      && !is_lazy()
      && computational_priority_flag == ComputationalPriority::Eager
      && !has(bound_variables, literal) 
      && !is_number(literal)
    ;
  }

  Abstraction::Abstraction(
    Variable binder,
    Expression* body,
    ComputationalPriority computational_priority
  ): Expression(computational_priority), binder(binder), body(body) {}

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
    if (is_normal_form) { 
      return { this, ReduceType::Null }; 
    }

    if (computational_priority_flag == ComputationalPriority::Lazy) {
      set_computational_priority(ComputationalPriority::Neutral);
    }

    ReduceType reduce_type;
    auto it = bound_variables.emplace(binder.get_literal());
    std::tie(body, reduce_type) = body->reduce(
      symbol_table,
      bound_variables
    );
    bound_variables.erase(it);

    if ((bool)reduce_type) {
      auto new_expr = new Abstraction(binder, body, computational_priority_flag);
      delete this;
      return { new_expr, reduce_type };
    }

    set_computational_priority(ComputationalPriority::Neutral);
    is_normal_form = true;
    return { this, reduce_type }; 
  }

  auto Abstraction::replace(
    Variable& variable,
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    [[unlikely]] if (variable == binder) {
      return { this, ReduceType::Null };
    }

    [[unlikely]] if (expression.is_variable_free(binder.get_literal())) {
      for (int i = 0;; i++) {
        std::string&& new_literal = index_to_string(i);
        [[likely]] if (
          !has(bound_variables, new_literal) 
          && new_literal != binder.get_literal()
          && !expression.is_variable_free(new_literal)
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

    ReduceType reduce_type;
    auto it = bound_variables.emplace(binder.get_literal());
    std::tie(body, reduce_type) = body->replace(
      variable,
      expression,
      bound_variables
    );
    bound_variables.erase(it);

    if ((bool)reduce_type) {
      is_normal_form = false;
      is_is_eager_flag_updated = false;
    }

    return { this, reduce_type };
  }

  auto Abstraction::apply(
    Expression& expression,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    auto it = bound_variables.emplace(binder.get_literal());
    auto result = body->replace(
      binder,
      expression,
      bound_variables
    ).first;
    result->set_computational_priority(computational_priority_flag);
    bound_variables.erase(it);

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
    return this->clone(computational_priority_flag);
  }
  auto Abstraction::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    auto result = new Abstraction(binder, body->clone());

    result->is_normal_form = is_normal_form;

    result->is_is_eager_flag_updated = is_is_eager_flag_updated;
    result->is_eager_flag = is_eager_flag;

    result->set_computational_priority(new_computational_priority);

    return result;
  }

  bool Abstraction::is_variable_free(const std::string& literal) {
    return binder.get_literal() != literal
      && body->is_variable_free(literal);
  }

  void Abstraction::update_eager_flag(
    std::multiset<std::string>& bound_variables
  ) {
    if (is_is_eager_flag_updated) { return; }
    is_is_eager_flag_updated = true;

    is_eager_flag = 
      !is_normal_form
      && computational_priority_flag == ComputationalPriority::Eager;
  }

  Application::Application(
    Expression* first,
    Expression* second,
    ComputationalPriority computational_priority
  ): Expression(computational_priority), first(first), second(second) {}

  void Application::delete_instance() {
    first->delete_instance();
    second->delete_instance();
    delete this;
  }

  auto Application::reduce_first(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<bool, ReduceType> {
    ReduceType reduce_type;
    std::tie(first, reduce_type) = first->reduce(symbol_table, bound_variables);

    if ((bool)reduce_type) {
      is_is_eager_flag_updated = false;
    }

    return { (bool)reduce_type, reduce_type };
  }

  auto Application::reduce_second(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<bool, ReduceType> {
    ReduceType reduce_type;
    std::tie(second, reduce_type) = second->reduce(symbol_table, bound_variables);

    if ((bool)reduce_type) {
      is_is_eager_flag_updated = false;
    }

    return { (bool)reduce_type, reduce_type };
  }

  auto Application::reduce(
    std::map<std::string, Expression*>& symbol_table,
    std::multiset<std::string>& bound_variables
  ) -> std::pair<Expression*, ReduceType> {
    if (is_normal_form) { 
      return { this, ReduceType::Null }; 
    }

    if (computational_priority_flag == ComputationalPriority::Lazy) {
      set_computational_priority(ComputationalPriority::Neutral);
    }

    if (first->is_eager(bound_variables)) {
      auto [reduced, reduce_type] = reduce_first(symbol_table, bound_variables);
      if (reduced) return { this, reduce_type };
    }
    if (second->is_eager(bound_variables)) {
      auto [reduced, reduce_type] = reduce_second(symbol_table, bound_variables);
      if (reduced) return { this, reduce_type };
    }

    auto [new_expr, reduce_type] = first->apply(*second, bound_variables);
    if ((bool)reduce_type) {
      new_expr->set_computational_priority(computational_priority_flag);
      delete this;
      return {new_expr, reduce_type};
    }

    if (first->is_lazy()) {
      auto [reduced, reduce_type] = reduce_second(symbol_table, bound_variables);
      if (reduced) return { this, reduce_type };

      std::tie(reduced, reduce_type) = reduce_first(symbol_table, bound_variables);
      if (reduced) return { this, reduce_type };
    }
    else {
      auto [reduced, reduce_type] = reduce_first(symbol_table, bound_variables);
      if (reduced) return { this, reduce_type };

      std::tie(reduced, reduce_type) = reduce_second(symbol_table, bound_variables);
      if (reduced) return { this, reduce_type };
    }

    set_computational_priority(ComputationalPriority::Neutral);
    is_normal_form = true;
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

    if ((bool)first_reduce_type || (bool) second_reduce_type) {
      is_normal_form = false;
      is_is_eager_flag_updated = false;
    }

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
    return this->clone(computational_priority_flag);
  }
  auto Application::clone(
    ComputationalPriority new_computational_priority
  ) -> Expression* {
    auto result = new Application(first->clone(), second->clone());

    result->is_normal_form = is_normal_form;
    
    result->is_is_eager_flag_updated = is_is_eager_flag_updated;
    result->is_eager_flag = is_eager_flag;

    result->set_computational_priority(new_computational_priority);

    return result;
  }

  bool Application::is_variable_free(const std::string& literal) {
    return first->is_variable_free(literal)
      || second->is_variable_free(literal);
  }

  void Application::update_eager_flag(
    std::multiset<std::string>& bound_variables
  ) {
    if (is_is_eager_flag_updated) { return; }
    is_is_eager_flag_updated = true;

    is_eager_flag = 
      !is_normal_form
      && !is_lazy()
      && (
        computational_priority_flag == ComputationalPriority::Eager
        || first->is_eager(bound_variables) 
        || second->is_eager(bound_variables)
      )
    ;
  }


  static auto generate_church_number_body(unsigned number) -> Expression* {
    [[unlikely]] if (number == 0) { return new Variable("x"); }

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
    switch (reduce_type) {
      [[unlikely]] case ReduceType::Alpha: return "alpha> ";
      [[likely]] case ReduceType::Beta: return "beta>  ";
      case ReduceType::Delta: return "delta> ";
      default: assert(false); return "";
    }
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
        ReduceType reduce_type;
        std::tie(expr, reduce_type) = expr->reduce(symbol_table, bound_variables);

        [[unlikely]] if (reduce_type == ReduceType::Null) { break; }

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