#pragma once

#include "db/UserDb.hpp"
#include "db/ArticleDb.hpp"
#include "db/CommentDb.hpp"
#include "utils/Config.hpp"

class TestDatabaseComponent {
public:

  OATPP_COMPONENT(std::shared_ptr<Config>, config);

  std::shared_ptr<oatpp::postgresql::ConnectionProvider> m_ConnectionProvider =
    std::make_shared<oatpp::postgresql::ConnectionProvider>(config->dbUrl);

  std::shared_ptr<oatpp::postgresql::ConnectionPool> m_ConnectionPool =
    oatpp::postgresql::ConnectionPool::createShared(
        m_ConnectionProvider,
        config->dbPool,
        std::chrono::seconds(config->dbTtl));

  std::shared_ptr<oatpp::postgresql::Executor> m_DbExecutor =
    std::make_shared<oatpp::postgresql::Executor>(m_ConnectionPool);

  OATPP_CREATE_COMPONENT(std::shared_ptr<UserDb>, userDb)([&] {
    return std::make_shared<UserDb>(m_DbExecutor);
  }());
  OATPP_CREATE_COMPONENT(std::shared_ptr<ArticleDb>, articleDb)([&] {
    return std::make_shared<ArticleDb>(m_DbExecutor);
  }());
  OATPP_CREATE_COMPONENT(std::shared_ptr<CommentDb>, commentDb)([&] {
    return std::make_shared<CommentDb>(m_DbExecutor);
  }());
};
