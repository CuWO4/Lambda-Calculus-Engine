#ifndef LAMBDA_H_
#define LAMBDA_H_

#include <memory>
#include <tuple>
#include <string>
#include <unordered_map>

namespace lambda {

  enum class Priority {
    Abstraction = 0,
    Application = 1,
    Variable = 2
  };

  class Variable;
  class Expression {
  public:
    virtual auto beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> = 0;

    virtual auto replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> = 0;

    virtual auto apply(
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> = 0;

    virtual auto delta_reduce(
      std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> = 0;

    virtual auto to_string() const -> std::string = 0;
    virtual auto debug(int indent) const -> std::string = 0;

    virtual auto get_priority() const -> Priority = 0;

    virtual auto clone() const -> std::unique_ptr<Expression> = 0;

    virtual ~Expression() = default;
  };

  class Variable: public Expression {
  public:
    Variable(std::string literal);

    Variable(Variable& other) = delete;
    Variable(Variable&& other) = default;
    Variable& operator=(Variable& other) = delete;
    Variable& operator=(Variable&& other) = default;

    bool operator==(const Variable& right) const;
    bool operator==(const Variable&& right) const;

    auto beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto apply(
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto delta_reduce(
      std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto to_string() const -> std::string override;
    auto debug(int indent) const -> std::string override;

    auto get_priority() const -> Priority override;

    auto clone_variable() const -> std::unique_ptr<Variable>;
    auto clone() const -> std::unique_ptr<Expression> override;

  private:
    std::string literal;
  };

  class Abstraction: public Expression {
  public:
    Abstraction(
      std::unique_ptr<Variable> binder,
      std::unique_ptr<Expression> body
    );

    Abstraction(Abstraction& other) = delete;
    Abstraction(Abstraction&& other) = default;
    Abstraction& operator=(Abstraction& other) = delete;
    Abstraction& operator=(Abstraction&& other) = default;

    auto beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto apply(
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto delta_reduce(
      std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto to_string() const -> std::string override;
    auto debug(int indent) const -> std::string override;

    auto get_priority() const -> Priority override;

    auto clone() const -> std::unique_ptr<Expression> override;

  private:
    std::unique_ptr<Variable> binder;
    std::unique_ptr<Expression> body;
  };

  class Application: public Expression {
  public:
    Application(
      std::unique_ptr<Expression> first,
      std::unique_ptr<Expression> second
    );

    Application(Application& other) = delete;
    Application(Application&& other) = default;
    Application& operator=(Application& other) = delete;
    Application& operator=(Application&& other) = default;

    auto beta_reduce() const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto replace(
      const std::unique_ptr<const Variable> variable,
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto apply(
      const std::unique_ptr<const Expression> expression
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto delta_reduce(
      std::unordered_map<std::string, std::unique_ptr<Expression>>& symbol_table
    ) const -> std::tuple<std::unique_ptr<Expression>, bool> override;

    auto to_string() const -> std::string override;
    auto debug(int indent) const -> std::string override;

    auto get_priority() const -> Priority override;

    auto clone() const -> std::unique_ptr<Expression> override;

  private:
    std::unique_ptr<Expression> first;
    std::unique_ptr<Expression> second;
  };

  auto generate_church_number(unsigned number) -> std::unique_ptr<Expression>;

  class Reducer {
  public:
    auto reduce(
      std::unique_ptr<Expression> expression
    ) -> std::tuple<std::string, std::unique_ptr<Expression>>;

    void register_symbol(std::string literal, std::unique_ptr<Expression> expression);

  private:
    std::unordered_map<std::string, std::unique_ptr<Expression>> symbol_table;
  };

}

#endif