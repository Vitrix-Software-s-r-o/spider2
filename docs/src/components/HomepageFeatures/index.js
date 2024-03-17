import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

import componentsJpeg from '/static/img/components.jpg'
import routingJpeg from '/static/img/routing.jpg'

const FeatureList = [
  {
    title: 'DRY',
    img: componentsJpeg,
    description: (
      <>
        Do not repeat yourself and build your app from reusable components.
      </>
    ),
  },
  {
    title: 'Powerful routing system',
    img: routingJpeg,
    description: (
      <>
        Spider2 provides a powerful routing system to build your app.
      </>
    ),
  },
  {
    title: 'Powered by Boost',
    Svg: require('@site/static/img/boost-logo.svg').default,
    description: (
      <>
        Spider2 is build on top of Boost.Beast, Boost.JSON and Boost.ASIO.
        It uses the ASIO asynchronous model to provide a scalable and efficient HTTP server.
      </>
    ),
  },
];

function Feature({Svg, img, title, description}) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center">
          {img && <img className={styles.featureSvg} src={img} alt={title}/> }
          {Svg && <Svg className={styles.featureSvg} role="img" />}
      </div>
      <div className="text--center padding-horiz--md">
        <Heading as="h3" className={"feature-heading"}>{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
