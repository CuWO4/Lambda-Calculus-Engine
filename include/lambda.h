#ifndef LAMBDA_H_
#define LAMBDA_H_

#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace lambda {

  // priority of computing, used for reducing
  enum class ComputationalPriority {
    Lazy = -1,
    Neutral = 0,
    Eager = 1
  };
  ComputationalPriority remove_lazy(ComputationalPriority);

  enum class ReduceType {
    Null = 0,
    Alpha = 1,
    Beta = 2,
    Delta = 3
  };

  // priority syntactically, used for printing
  enum class Priority {
    Abstraction = 0,
    Application = 1,
    Variable = 2
  };

  class Variable;
  class Expression {
  public:
    // delete this recursively
    // crash when this is not allocated dynamically
    virtual void delete_instance() = 0;

    // beta and delta reduce
    virtual auto reduce(
      std::unordered_map<std::string, Expression*>& symbol_table,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> = 0;

    virtual auto replace(
      Variable& variable,
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> = 0;

    virtual auto apply(
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> = 0;

    virtual auto to_string() -> std::string = 0;

    virtual auto get_priority() -> Priority = 0;

    virtual auto clone() -> Expression* = 0;
    virtual auto clone(
      ComputationalPriority new_computational_priority
    ) -> Expression* = 0;

    auto get_free_variables() -> std::unordered_set<std::string>&;

    virtual bool is_eager(
      std::unordered_multiset<std::string>& bound_variables
    ) = 0;
    virtual bool is_lazy() = 0;
    void set_computational_priority(
      ComputationalPriority computational_priority
    );

  protected:
    ComputationalPriority computational_priority_flag;
    std::unordered_set<std::string> free_variables;

    virtual ~Expression() = default;
  };

  class Variable: public Expression {
  public:
    Variable(
      std::string literal, 
      ComputationalPriority computational_priority = ComputationalPriority::Neutral
    );
    ~Variable() = default;

    void delete_instance() override;

    Variable(Variable& other) = default;
    Variable(Variable&& other) = default;
    Variable& operator=(Variable& other) = default;
    Variable& operator=(Variable&& other) = default;

    bool operator==(Variable& right);
    bool operator==(Variable&& right);

    auto get_literal() -> std::string&;

    auto reduce(
      std::unordered_map<std::string, Expression*>& symbol_table,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto replace(
      Variable& variable,
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto apply(
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto to_string() -> std::string override;

    auto get_priority() -> Priority override;

    auto clone() -> Expression* override;
    auto clone(
      ComputationalPriority new_computational_priority
    ) -> Expression* override;

    bool is_eager(
      std::unordered_multiset<std::string>& bound_variables
    ) override;
    bool is_lazy() override;

  private:
    std::string literal;
  };

  class Abstraction: public Expression {
  public:
    // to make sure all instance is alloced dynamically
    static Abstraction* get_instance(
      Variable binder,
      Expression* body,
      ComputationalPriority computational_priority = ComputationalPriority::Neutral
    );
    void delete_instance() override;

    Abstraction(Abstraction& other) = default;
    Abstraction(Abstraction&& other) = default;
    Abstraction& operator=(Abstraction& other) = default;
    Abstraction& operator=(Abstraction&& other) = default;

    auto alpha_reduce(
      Variable to
    ) -> Abstraction*;

    auto reduce(
      std::unordered_map<std::string, Expression*>& symbol_table,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto replace(
      Variable& variable,
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto apply(
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto to_string() -> std::string override;

    auto get_priority() -> Priority override;

    auto clone() -> Expression* override;
    auto clone(
      ComputationalPriority new_computational_priority
    ) -> Expression* override;

    bool is_eager(
      std::unordered_multiset<std::string>& bound_variables
    ) override;
    bool is_lazy() override;

  private:
    Variable binder;
    Expression* body;

    Abstraction(
      Variable binder,
      Expression* body,
      ComputationalPriority computational_priority
    );
    // delete non-recursively
    ~Abstraction() = default;
  };

  class Application: public Expression {
  public:
    static Application* get_instance(
      Expression* first,
      Expression* second,
      ComputationalPriority computational_priority = ComputationalPriority::Neutral
    );
    void delete_instance() override;

    Application(Application& other) = default;
    Application(Application&& other) = default;
    Application& operator=(Application& other) = default;
    Application& operator=(Application&& other) = default;

    auto reduce(
      std::unordered_map<std::string, Expression*>& symbol_table,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto replace(
      Variable& variable,
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto apply(
      Expression& expression,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<Expression*, ReduceType> override;

    auto to_string() -> std::string override;

    auto get_priority() -> Priority override;

    auto clone() -> Expression* override;
    auto clone(
      ComputationalPriority new_computational_priority
    ) -> Expression* override;

    bool is_eager(
      std::unordered_multiset<std::string>& bound_variables
    ) override;
    bool is_lazy() override;

  private:
    Expression* first;
    Expression* second;

    Application(
      Expression* first,
      Expression* second,
      ComputationalPriority computational_priority
    );
    ~Application() = default;

    auto reduce_first(
      std::unordered_map<std::string, Expression*>& symbol_table,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<bool, ReduceType>;

    auto reduce_second(
      std::unordered_map<std::string, Expression*>& symbol_table,
      std::unordered_multiset<std::string>& bound_variables
    ) -> std::pair<bool, ReduceType>;

    void update_free_variables();
  };

  auto generate_church_number(unsigned number) -> Expression*;

  class Reducer {
  public:
    auto reduce(
      Expression* expression
    ) -> std::pair<std::string, Expression*>;

    void register_symbol(std::string literal, Expression* expression);

    ~Reducer();

  private:
    std::unordered_map<std::string, Expression*> symbol_table;
  };

}

#endif