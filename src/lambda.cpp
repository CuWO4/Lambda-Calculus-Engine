#include "lambda.h"

#include <ctime>
#include <functional>

namespace lambda {

  static auto build_indent(int indent) { 
    auto res = std::string("");
    for (int i = 0;  i < indent; i++) res += " ";
    return res;
  }

  template <typename T>
  static auto operator+(
    std::unordered_set<T> const& a,
    std::unordered_set<T> const& b
  ) -> std::unordered_set<T> {
    auto result = a;
    result.insert(b.begin(), b.end());
    return result;
  }

  template <template<typename...> typename Container, typename T, typename... Ts>
  static bool has(const Container<T, Ts...>& container, const T& element) {
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


  auto Expression::get_free_variables() const -> std::unordered_set<std::string> const& {
    return free_variables;
  }

  bool Expression::get_computational_priority() const { 
    return computational_priority_flag; 
  }

  void Expression::set_computational_priority(bool computational_priority) {
    computational_priority_flag = computational_priority;
  }


  Variable::Variable(std::string literal, bool computational_priority): literal(literal) {
    free_variables = { literal };
    computational_priority_flag = computational_priority;
  }

  bool Variable::operator==(const Variable& right) const { 
    return literal == right.literal; 
  }
  bool Variable::operator==(const Variable&& right) const { 
    return literal == right.literal; 
  }

  auto Variable::get_literal() -> std::string const& { return literal; }

  auto Variable::reduce(
    std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    if (!has(bound_variables, literal) && has(symbol_table, literal)) {
      return std::make_tuple(
        symbol_table.find(literal)->second->clone(this->computational_priority_flag),
        ReduceType::DeltaReduce
      );
    }

    return std::make_tuple(
      this->clone(false),
      ReduceType::NoReduce
    );
  }

  auto Variable::replace(
    const std::unique_ptr<const Variable> variable,
    const std::unique_ptr<const Expression> expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    if (*this == *variable) {
      return std::make_tuple(
        expression->clone(this->computational_priority_flag),
        ReduceType::BetaReduce
      );
    }
    
    return std::make_tuple(
      this->clone(),
      ReduceType::NoReduce
    );
  }

  auto Variable::apply(
    const std::unique_ptr<const Expression> expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {  
    return std::make_tuple(
      this->clone(),
      ReduceType::NoReduce
    );
  }

  auto Variable::to_string() const -> std::string { 
    return literal; 
  }

  auto Variable::debug(int indent) const -> std::string { return build_indent(indent) + literal; }

  auto Variable::get_priority() const -> Priority {
    return Priority::Variable;
  }

  auto Variable::clone_variable() const -> std::unique_ptr<Variable> {
    return std::make_unique<Variable>(
      literal, 
      computational_priority_flag
    );
  }

  auto Variable::clone_variable(
    bool new_computational_priority
  ) const -> std::unique_ptr<Variable> {
    return std::make_unique<Variable>(
      literal, 
      new_computational_priority
    );
  }

  auto Variable::clone() const -> std::unique_ptr<Expression> {
    return this->clone_variable();
  }
  auto Variable::clone(
    bool new_computational_priority
  ) const -> std::unique_ptr<Expression> {
    return this->clone_variable(new_computational_priority);
  }

  bool Variable::is_computational_priority(
    std::unordered_multiset<std::string>&& bound_variables
  ) const {
    return !has(bound_variables, literal) && computational_priority_flag;
  }


  Abstraction::Abstraction(
    std::unique_ptr<Variable> binder,
    std::unique_ptr<Expression> body,
    bool computational_priority
  ): binder(std::move(binder)), body(std::move(body)) {
    free_variables = this->body->get_free_variables();
    free_variables.erase(this->binder->get_literal());
    computational_priority_flag = computational_priority;
  }

  auto Abstraction::alpha_reduce(
    const std::unique_ptr<const Variable> to
  ) const -> std::unique_ptr<Expression> {
    return std::make_unique<Abstraction>(
      to->clone_variable(),
      std::get<0>(
        body->replace(
          binder->clone_variable(), 
          to->clone_variable(), 
          {} // do not care and should not care
        )
      ),
      computational_priority_flag
    );
  }

  auto Abstraction::reduce(
    std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    bound_variables.emplace(binder->get_literal());
    auto [new_body, reduce_type] = body->reduce(symbol_table, std::move(bound_variables));
    bound_variables.erase(bound_variables.find(binder->get_literal()));

    return std::make_tuple(
      std::make_unique<Abstraction>(
        binder->clone_variable(),
        std::move(new_body),
        (bool)reduce_type && computational_priority_flag
      ),
      reduce_type
    );
  }

  auto Abstraction::replace(
    const std::unique_ptr<const Variable> variable,
    const std::unique_ptr<const Expression> expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    if (*variable == *binder) {
      return std::make_tuple(
        this->clone(),
        ReduceType::NoReduce
      );
    }

    if (has(expression->get_free_variables(), binder->get_literal())) {
      for (int i = 0;; i++) {
        std::string new_literal = index_to_string(i);
        if (!has(bound_variables, new_literal)) {
          return this->alpha_reduce(
            std::make_unique<Variable>(new_literal)
          )->replace(
            variable->clone_variable(), 
            expression->clone(), 
            std::move(bound_variables)
          );
        }
      }
    }

    bound_variables.emplace(binder->get_literal());
    auto [new_body, reduce_type] = body->replace(
      variable->clone_variable(), 
      expression->clone(),
      std::move(bound_variables)
    );
    bound_variables.erase(bound_variables.find(binder->get_literal()));

    return std::make_tuple(
      std::make_unique<Abstraction>(
        binder->clone_variable(),
        std::move(new_body),
        computational_priority_flag
      ),
      reduce_type
    );
  }

  auto Abstraction::apply(
    const std::unique_ptr<const Expression> expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    bound_variables.emplace(binder->get_literal());
    auto result = std::get<0>(
      body->replace(
        binder->clone_variable(), 
        expression->clone(), 
        std::move(bound_variables)
      )
    );
    result->set_computational_priority(computational_priority_flag);
    bound_variables.erase(bound_variables.find(binder->get_literal()));

    return std::make_tuple(
      std::move(result),
      ReduceType::BetaReduce
    );
  }

  auto Abstraction::to_string() const -> std::string {
    return 
      "\\" + binder->to_string() 
      + "." + (body->get_priority() > Priority::Abstraction ? " " : "") 
      + body->to_string()
    ;
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
    return std::make_unique<Abstraction>(
      binder->clone_variable(), 
      body->clone(), 
      computational_priority_flag
    );
  }
  auto Abstraction::clone(
    bool new_computational_priority
  ) const -> std::unique_ptr<Expression> {
    return std::make_unique<Abstraction>(
      binder->clone_variable(), 
      body->clone(), 
      new_computational_priority
    );
  }

  bool Abstraction::is_computational_priority(
    std::unordered_multiset<std::string>&& bound_variables
  ) const {
    for(auto free_variable: free_variables) {
      if (has(bound_variables, free_variable)) { return false; }
    }
    return computational_priority_flag;
  }


  Application::Application(
    std::unique_ptr<Expression> first,
    std::unique_ptr<Expression> second,
    bool computational_priority
  ): first(std::move(first)), second(std::move(second)) {
    free_variables = this->first->get_free_variables() 
      + this->second->get_free_variables();
    computational_priority_flag = computational_priority;
  }

  auto Application::reduce(
    std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    if (first->is_computational_priority(std::move(bound_variables))) {
      auto [new_first, reduce_type] = first->reduce(symbol_table, std::move(bound_variables));
      if ((bool)reduce_type) {
        new_first->set_computational_priority(first->get_computational_priority());
        return std::make_tuple(
          std::make_unique<Application>(
            std::move(new_first),
            second->clone(),
            computational_priority_flag
          ),
          reduce_type
        );
      } 
    }
    else if (second->is_computational_priority(std::move(bound_variables))) {
      auto [new_second, reduce_type] = second->reduce(symbol_table, std::move(bound_variables));
      if ((bool)reduce_type) {
        new_second->set_computational_priority(second->get_computational_priority());
        return std::make_tuple(
          std::make_unique<Application>(
            first->clone(),
            std::move(new_second),
            computational_priority_flag
          ),
          reduce_type
        );
      } 
    }
    {
      auto [new_expression, reduce_type] = first->apply(second->clone(), std::move(bound_variables));
      if ((bool)reduce_type) {
        new_expression->set_computational_priority(computational_priority_flag);
        return std::make_tuple(std::move(new_expression), reduce_type);
      }
    }
    {
      auto [new_first, reduce_type] = first->reduce(symbol_table, std::move(bound_variables));
      if ((bool)reduce_type) {
        new_first->set_computational_priority(first->get_computational_priority());
        return std::make_tuple(
          std::make_unique<Application>(
            std::move(new_first),
            second->clone(),
            computational_priority_flag
          ),
          reduce_type
        );
      } 
    }
    {
      auto [new_second, reduce_type] = second->reduce(symbol_table, std::move(bound_variables));
      if ((bool)reduce_type) {
        new_second->set_computational_priority(second->get_computational_priority());
        return std::make_tuple(
          std::make_unique<Application>(
            first->clone(),
            std::move(new_second),
            computational_priority_flag
          ),
          reduce_type
        );
      } 
    }
    return std::make_tuple(
      this->clone(false),
      ReduceType::NoReduce
    );
  }

  auto Application::replace(
    const std::unique_ptr<const Variable> variable,
    const std::unique_ptr<const Expression> expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    auto [new_first, first_reduce_type] = first->replace(
      variable->clone_variable(), 
      expression->clone(), 
      std::move(bound_variables)
    );
    if (first_reduce_type == ReduceType::AlphaReduce) {
      return std::make_tuple(
        std::make_unique<Application>(
          std::move(new_first),
          second->clone(),
          computational_priority_flag
        ),
        first_reduce_type
      );
    }

    auto [new_second, second_reduce_type] = second->replace(
      variable->clone_variable(), 
      expression->clone(), 
      std::move(bound_variables)
    );
    if (second_reduce_type == ReduceType::AlphaReduce) {
      return std::make_tuple(
        std::make_unique<Application>(
          first->clone(),
          std::move(new_second),
          computational_priority_flag
        ),
        second_reduce_type
      );
    }

    return std::make_tuple(
      std::make_unique<Application>(
        std::move(new_first),
        std::move(new_second),
        computational_priority_flag
      ),
      (bool)first_reduce_type
        ? first_reduce_type
        : second_reduce_type
    );
  }

  auto Application::apply(
    const std::unique_ptr<const Expression> expression,
    std::unordered_multiset<std::string>&& bound_variables
  ) const -> std::tuple<std::unique_ptr<Expression>, ReduceType> {
    return std::make_tuple(
      this->clone(),
      ReduceType::NoReduce
    );
  }

  auto Application::to_string() const -> std::string {
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
    return std::make_unique<Application>(
      first->clone(), 
      second->clone(),
      computational_priority_flag
    );
  }
  auto Application::clone(
    bool new_computational_priority
  ) const -> std::unique_ptr<Expression> {
    return std::make_unique<Application>(
      first->clone(), 
      second->clone(), 
      new_computational_priority
    );
  }

  bool Application::is_computational_priority(
    std::unordered_multiset<std::string>&& bound_variables
  ) const {
    return computational_priority_flag 
      || first->is_computational_priority(std::move(bound_variables))
      || second->is_computational_priority(std::move(bound_variables))
    ;
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

  static std::string reduce_type_to_header(ReduceType reduce_type) {
    if (reduce_type == ReduceType::AlphaReduce) { return "alpha> "; }
    else if (reduce_type == ReduceType::BetaReduce) { return "beta>  "; }
    else if (reduce_type == ReduceType::DeltaReduce) { return "delta> "; }
    else /* reduce_type == ReduceType::NoReduce */ { return ""; }
  }

  clock_t msec_count(std::function<void(void)> func) {
    auto start_time = clock();
    func();
    auto end_time = clock();
    return (end_time - start_time) / (double)CLOCKS_PER_SEC * 1000;
  }

  auto Reducer::reduce(
     std::unique_ptr<Expression> expression
  ) -> std::tuple<std::string, std::unique_ptr<Expression>> {

    auto result_string = expression->to_string() + "\n";
    auto expr = expression->clone();

    unsigned long long step;

    auto msec = msec_count(
      [&]() {
        for (step = 0;; step++) {
          auto [new_expr, reduce_type] = expr->reduce(symbol_table, {});

          if (reduce_type == ReduceType::NoReduce) { break; }

          result_string += reduce_type_to_header(reduce_type) + new_expr->to_string() + "\n";
          expr = std::move(new_expr);
        }
      }
    );

    result_string += "\nto be sought:    " + expression->to_string() + "\n"
                  +    "result:          " + expr->to_string() + "\n"
                  +    "step taken:      " + std::to_string(step) + "\n"
                  +    "character count: " + std::to_string(result_string.length()) + "\n"
                  +    "time cost:       " + std::to_string(msec) + "ms" +"\n";
    
    return std::make_tuple(result_string, std::move(expr));
  }


  void Reducer::register_symbol(
    std::string literal, 
    std::unique_ptr<Expression> expression
  ) {
    symbol_table[literal] = std::move(expression);
  }

}