#include "lambda.h"

namespace lambda {

  static auto build_indent(int indent) { 
    auto res = std::string("");
    for (int i = 0;  i < indent; i++) res += " ";
    return res;
  }


  Variable::Variable(std::string literal): literal(literal) {}

  bool Variable::operator==(const Variable& right) const { return literal == right.literal; }
  bool Variable::operator==(const Variable&& right) const { return literal == right.literal; }

  auto Variable::beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> {
    return std::make_tuple(this->clone(), false);
  }

  auto Variable::replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    if (*variable == *this) {
      return std::make_tuple(expression->clone(), true);
    }
    else {
      return std::make_tuple(this->clone(), false);
    }
  }

  auto Variable::apply(
    const std::unique_ptr<const Expression> expression
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    return std::make_tuple(this->clone(), false);
  }

  auto Variable::delta_reduce(
    std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    if (symbol_table.find(literal) != symbol_table.end()) {
      return std::make_tuple(
        symbol_table.find(literal)->second->clone(),
        true
      );
    }
    else { return std::make_tuple(this->clone(), false); }
  }

  auto Variable::to_string() const -> std::string { return literal; }

  auto Variable::debug(int indent) const -> std::string { return build_indent(indent) + literal; }

  auto Variable::get_priority() const -> Priority {
    return Priority::Variable;
  }

  auto Variable::clone_variable() const -> std::unique_ptr<Variable> {
    return std::make_unique<Variable>(literal);
  }

  auto Variable::clone() const -> std::unique_ptr<Expression> {
    return this->clone_variable();
  }


  Abstraction::Abstraction(
    std::unique_ptr<Variable> variable,
    std::unique_ptr<Expression> body
  ): binder(std::move(variable)), body(std::move(body)) {}

  auto Abstraction::beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> {
    auto [body, is_changed] = this->body->beta_reduce();
    return std::make_tuple(
      std::make_unique<Abstraction>(binder->clone_variable(), std::move(body)),
      is_changed
    );
  }

  auto Abstraction::replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    if (*this->binder == *variable) {
      return std::make_tuple(this->clone(), false);
    }

    auto [body, is_changed] = this->body->replace(
      variable->clone_variable(),
      expression->clone()
    );
    return std::make_tuple(
      std::make_unique<Abstraction>(this->binder->clone_variable(), std::move(body)),
      is_changed
    );
  }

  auto Abstraction::apply(
    const std::unique_ptr<const Expression> expression
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    auto [body, is_changed] = this->body->replace(binder->clone_variable(), expression->clone());
    return std::make_tuple(std::move(body), true);
  }

  auto Abstraction::delta_reduce(
    std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    auto [body, is_changed] = this->body->delta_reduce(symbol_table);
    return std::make_tuple(
      std::make_unique<Abstraction>(this->binder->clone_variable(), std::move(body)),
      is_changed
    );
  }

  auto Abstraction::to_string() const -> std::string {
    return "\\" + binder->to_string() + "." + body->to_string();
  }

  auto Abstraction::debug(int indent) const -> std::string {
    auto result = std::string("");

    result += build_indent(indent) + "abst {\n";
    result += binder->debug(indent + 1) + "\n";
    result += body->debug(indent + 1) + "\n";
    result += build_indent(indent) + "}";

    return result;
  }

  auto Abstraction::get_priority() const -> Priority {
    return Priority::Abstraction;
  }

  auto Abstraction::clone() const -> std::unique_ptr<Expression> {
    return std::make_unique<Abstraction>(binder->clone_variable(), body->clone());
  }

  Application::Application(
    std::unique_ptr<Expression> first,
    std::unique_ptr<Expression> second
  ): first(std::move(first)), second(std::move(second)) {}

  auto Application::beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> {
    {
      auto [result, is_changed] = first->apply(second->clone());
      if (is_changed) { return std::make_tuple(std::move(result), true); }
    }
    {
      auto [first, is_changed] = this->first->beta_reduce();
      if (is_changed) {
        return std::make_tuple(std::make_unique<Application>(first->clone(), second->clone()), true);
      }
    }
    {
      auto [second, is_changed] = this->second->beta_reduce();
      if (is_changed) {
        return std::make_tuple(std::make_unique<Application>(first->clone(), second->clone()), true);
      }
    }
    return std::make_tuple(this->clone(), false);
  }

  auto Application::replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    std::unique_ptr<Expression> first_result, second_result;
    bool is_changed_result = false;
    {
      auto [first, is_changed] = this->first->replace(variable->clone_variable(), expression->clone());
      first_result = std::move(first);
      is_changed_result |= is_changed;
    }
    {
      auto [second, is_changed] = this->second->replace(variable->clone_variable(), expression->clone());
      second_result = std::move(second);
      is_changed_result |= is_changed;
    }
    return std::make_tuple(
      std::make_unique<Application>(std::move(first_result), std::move(second_result)), 
      is_changed_result
    );
  }

  auto Application::apply (
    const std::unique_ptr<const Expression> expression
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    return std::make_tuple(this->clone(), false);
  }

  auto Application::delta_reduce(
    std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
  ) const -> std::tuple<std::unique_ptr<Expression>, bool> {
    {
      auto [first, is_changed] = this->first->delta_reduce(symbol_table);
      if (is_changed) {
        return std::make_tuple(std::make_unique<Application>(std::move(first), second->clone()), true);
      }
    }
    {
      auto [second, is_changed] = this->second->delta_reduce(symbol_table);
      if (is_changed) {
        return std::make_tuple(std::make_unique<Application>(first->clone(), std::move(second)), true);
      }
    }
    return std::make_tuple(this->clone(), false);
  }

  auto Application::to_string() const -> std::string {
    auto result = std::string("");

    if (first->get_priority() < get_priority()) { result += "(" + first->to_string() + ")"; }
    else { result += first->to_string(); }

    result += " ";

    if (second->get_priority() <= get_priority()) { result += "(" + second->to_string() + ")"; }
    else { result += second->to_string(); }

    return result;
  }

  auto Application::debug(int indent) const -> std::string {
    auto result = std::string("");

    result += build_indent(indent) + "appl {\n";
    result += first->debug(indent + 1) + "\n";
    result += second->debug(indent + 1) + "\n";
    result += build_indent(indent) + "}";

    return result;
  }

  auto Application::get_priority() const -> Priority {
    return Priority::Application;
  }

  auto Application::clone() const -> std::unique_ptr<Expression> {
    return std::make_unique<Application>(first->clone(), second->clone());
  }

  static auto generate_church_number_body(unsigned number) -> std::unique_ptr<Expression> {
    if (number == 0) { return std::make_unique<Variable>("x"); }

    return std::make_unique<Application>(
      std::make_unique<Variable>("f"),
      generate_church_number_body(number - 1)
    );
  }

  auto generate_church_number(unsigned number) -> std::unique_ptr<Expression> {
    return std::make_unique<Abstraction>(
      std::make_unique<Variable>("f"),
      std::make_unique<Abstraction>(
        std::make_unique<Variable>("x"),
        generate_church_number_body(number)
      )
    );
  }

  auto Reducer::reduce(
     std::unique_ptr<Expression> expression
  ) -> std::tuple<std::string, std::unique_ptr<Expression>> {

    auto result_string = expression->to_string() + "\n";
    auto expr = std::move(expression);

    bool is_changed_global = true;

    while (is_changed_global) {
      is_changed_global = false;

      {
        auto [new_expr, is_changed] = expr->beta_reduce();
        expr = std::move(new_expr);

        if (is_changed) { 
          is_changed_global = true;
          result_string += "beta>\t" + expr->to_string() + "\n";
          continue; 
        }
      }

      {
        auto [new_expr, is_changed] = expr->delta_reduce(symbol_table);
        expr = std::move(new_expr);

        if (is_changed) { 
          is_changed_global = true;
          result_string += "delta>\t" + expr->to_string() + "\n";
          continue; 
        }
      }
    }
    
    return std::make_tuple(result_string, std::move(expr));
  }

  void Reducer::register_symbol(
    std::string literal, 
    std::unique_ptr<Expression> expression
  ) {
    symbol_table[literal] = std::move(expression);
  }

}