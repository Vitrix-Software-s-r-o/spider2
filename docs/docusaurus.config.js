// @ts-check
// `@type` JSDoc annotations allow editor autocompletion and type checking
// (when paired with `@ts-check`).
// There are various equivalent ways to declare your Docusaurus config.
// See: https://docusaurus.io/docs/api/docusaurus-config

import {themes as prismThemes} from 'prism-react-renderer';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'Spider2',
  tagline: 'Ergonomic HTTP routing C++ library build on top of Boost.Beast and Boost.JSON',
  favicon: 'img/favicon.ico',

  // Set the production url of your site here
  url: 'https://spider2.vitrix.cz',
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: '/',

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: 'Vitrix-Software-s-r-o', // Usually your GitHub org/user name.
  projectName: 'spider2', // Usually your repo name.

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'warn',

  // Even if you don't use internationalization, you can use this field to set
  // useful metadata like html lang. For example, if your site is Chinese, you
  // may want to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          sidebarPath: './sidebars.js',
          // Please change this to your repo.
          // Remove this to remove the "edit this page" links.
          editUrl:
            'https://github.com/Vitrix-Software-s-r-o/spider2/docs',
        },
        blog: {
          showReadingTime: true,
          // Please change this to your repo.
          // Remove this to remove the "edit this page" links.
          editUrl:
            'https://github.com/Vitrix-Software-s-r-o/spider2',
        },
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      // Replace with your project's social card
      image: 'img/docusaurus-social-card.jpg',
      navbar: {
        title: 'spider2 C++',
        logo: {
          alt: 'spider2 logo',
          src: 'img/logo.jpg',
        },
        items: [
          {
            type: 'docSidebar',
            sidebarId: 'tutorialSidebar',
            position: 'left',
            label: 'Tutorial',
          },
          {to: '/blog', label: 'Blog', position: 'left'},
          {
            href: 'https://github.com/Vitrix-Software-s-r-o/spider2',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Basics',
            items: [
              {
                label: 'Tutorial',
                to: '/docs/intro',
              },
              {
                label: 'Building APIs',
                to: '/docs/apis',
              },
              {
                label: 'Middleware',
                to: '/docs/middleware',
              }
            ],
          },
          {
            title: 'Architecture',
            items: [
              {
                label: 'Routing',
                to: '/docs/routing',
              },
              {
                label: 'Composition',
                to: '/docs/composition',
              },
              {
                label: 'Middleware',
                to: '/docs/middleware',
              }
            ],
          },
          {
            title: 'More',
            items: [
              {
                label: 'Blog',
                to: '/blog',
              },
              {
                label: 'GitHub',
                href: 'https://github.com/Vitrix-Software-s-r-o/spider2',
              },
              {
                label: 'About us',
                href: 'https://www.vitrix.cz',
              }
            ],
          },
        ],
        copyright: `Copyright © ${new Date().getFullYear()} Vitrix Software, s.r.o. | Built with Docusaurus.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
      },
    }),
};

export default config;
